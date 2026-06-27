#include "cpu.h"
#include "bus.h"
#include <stdio.h>
#include <assert.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <string.h>

#define ACTION_READ 0x01
#define ACTION_WRITE 0x02
#define ACTION_FETCH 0x04

typedef struct {
    uint8_t actions;
    uint8_t size;
    uint32_t addr;
    uint32_t val;
} Cycle;

typedef struct {
    uint32_t R[32];
    uint32_t hi;
    uint32_t lo;
    uint32_t EPC;
    uint32_t CAUSE;
    uint32_t PC;
    struct {
        bool load_slot;
        bool load_take;
        uint32_t load_target;
        uint32_t branch_target;
        uint32_t branch_val;
    } delay;
} State;

Cycle *cycles;
uint8_t cycle_count;
uint8_t cycle_index;

void set_state(State *state, struct json_object *json_state) {
    // r is stored in the JSON as an array of 32 integers
    struct json_object *r_array = json_object_object_get(json_state, "R");
    if (r_array && json_object_get_type(r_array) == json_type_array) {
        for (int i = 0; i < 32; i++) {
            struct json_object *reg_value = json_object_array_get_idx(r_array, i);
            if (reg_value) {
                state->R[i] = json_object_get_uint64(reg_value);
            }
        }
    }

    struct json_object *hi_value = json_object_object_get(json_state, "hi");
    if (hi_value) {
        state->hi = json_object_get_uint64(hi_value);
    }

    struct json_object *lo_value = json_object_object_get(json_state, "lo");
    if (lo_value) {
        state->lo = json_object_get_uint64(lo_value);
    }

    struct json_object *epc_value = json_object_object_get(json_state, "EPC");
    if (epc_value) {
        state->EPC = json_object_get_uint64(epc_value);
    }

    struct json_object *cause_value = json_object_object_get(json_state, "CAUSE");
    if (cause_value) {
        state->CAUSE = json_object_get_uint64(cause_value);
    }

    struct json_object *pc_value = json_object_object_get(json_state, "PC");
    if (pc_value) {
        state->PC = json_object_get_uint64(pc_value);
    }

    struct json_object *delay_obj = json_object_object_get(json_state, "delay");
    if (delay_obj && json_object_get_type(delay_obj) == json_type_object) {
        struct json_object *load_value = json_object_object_get(delay_obj, "load");
        if (load_value) {
            state->delay.load_slot = json_object_get_boolean(json_object_object_get(load_value, "slot"));
            state->delay.load_take = json_object_get_boolean(json_object_object_get(load_value, "take"));
            state->delay.load_target = json_object_get_uint64(json_object_object_get(load_value, "target"));
        }
        struct json_object *branch_value = json_object_object_get(delay_obj, "branch");
        if (branch_value) {
            state->delay.branch_target = json_object_get_uint64(json_object_object_get(branch_value, "target"));
            state->delay.branch_val = json_object_get_uint64(json_object_object_get(branch_value, "val"));
        }
    }
}

void set_state_from_cpu(State *state, Cpu *cpu) {
    for (int i = 0; i < 32; i++) {
        state->R[i] = cpu->r[i];
    }
    state->hi = cpu->hi;
    state->lo = cpu->lo;
    state->EPC = cpu->cop0.epc;
    state->CAUSE = cpu->cop0.cause;
    state->PC = cpu->pc;

    state->delay.load_slot = cpu->in_delay_slot;
    state->delay.load_take = cpu->branch_taken;
    state->delay.load_target = cpu->branch_target;
    state->delay.branch_target = cpu->load_reg;
    state->delay.branch_val = cpu->load_value;
}

void set_cpu_from_state(Cpu *cpu, State *state) {
    for (int i = 0; i < 32; i++) {
        cpu->r[i] = state->R[i];
    }
    cpu->hi = state->hi;
    cpu->lo = state->lo;
    cpu->cop0.epc = state->EPC;
    cpu->cop0.cause = state->CAUSE;
    cpu->pc = state->PC;
    cpu->next_pc = state->PC + 4;
    cpu->next_exc_code = 0; // Clear any pending exception code

    cpu->in_delay_slot = state->delay.load_slot;
    cpu->branch_taken = state->delay.load_take;
    cpu->branch_target = state->delay.load_target;
    cpu->next_load_reg = 0;
    cpu->next_load_value = 0;
    cpu->load_reg = state->delay.branch_target;
    cpu->load_value = state->delay.branch_val;
}

void set_cycles(Cycle *cycle_array, struct json_object *json_cycles) {
    free(cycles);
    cycles = NULL;
    cycle_count = 0;
    cycle_index = 0;

    // cycles is stored in the JSON as an array of objects
    if (json_cycles && json_object_get_type(json_cycles) == json_type_array) {
        cycle_count = json_object_array_length(json_cycles);
        cycles = malloc(sizeof(Cycle) * cycle_count);

        for (int i = 0; i < cycle_count; i++) {
            struct json_object *cycle_obj = json_object_array_get_idx(json_cycles, i);
            if (cycle_obj && json_object_get_type(cycle_obj) == json_type_object) {
                struct json_object *actions = json_object_object_get(cycle_obj, "actions");
                struct json_object *sz = json_object_object_get(cycle_obj, "sz");
                struct json_object *addr = json_object_object_get(cycle_obj, "addr");
                struct json_object *val = json_object_object_get(cycle_obj, "val");

                if (actions) cycles[i].actions = json_object_get_uint64(actions);
                if (sz) cycles[i].size = json_object_get_uint64(sz);
                if (addr) cycles[i].addr = json_object_get_uint64(addr);
                if (val) cycles[i].val = json_object_get_uint64(val);
            }
        }
    }
}

uint32_t consume_read_cycle(uint8_t action, uint8_t size, uint32_t addr) {
    if (cycle_index >= cycle_count) {
        fprintf(stderr, "No more cycles to consume\n");
        return 0;
    }

    Cycle *current_cycle = &cycles[cycle_index];

    if (current_cycle->actions != action || current_cycle->size != size || current_cycle->addr != addr) {
        fprintf(stderr, "Cycle mismatch at index %d\n", cycle_index);
        return 0;
    }

    cycle_index++;
    return current_cycle->val;
}

void consume_write_cycle(uint8_t action, uint8_t size, uint32_t addr, uint32_t val) {
    if (cycle_index >= cycle_count) {
        fprintf(stderr, "No more cycles to consume\n");
        return;
    }

    Cycle *current_cycle = &cycles[cycle_index];

    if (current_cycle->actions != action || current_cycle->size != size || current_cycle->addr != addr || current_cycle->val != val) {
        fprintf(stderr, "Cycle mismatch at index %d\n", cycle_index);
        return;
    }

    cycle_index++;
}

uint8_t bus_read8_mock(Bus *bus, uint32_t addr) {
    return consume_read_cycle(ACTION_READ, 1, addr);
}
uint16_t bus_read16_mock(Bus *bus, uint32_t addr) {
    return consume_read_cycle(ACTION_READ, 2, addr);
}
uint32_t bus_read32_mock(Bus *bus, uint32_t addr) {
    return consume_read_cycle(ACTION_READ, 4, addr);
}
uint32_t bus_fetch32_mock(Bus *bus, uint32_t addr) {
    return consume_read_cycle(ACTION_FETCH, 4, addr);
}
void bus_write8_mock(Bus *bus, uint32_t addr, uint8_t value) {
    consume_write_cycle(ACTION_WRITE, 1, addr, value);
}
void bus_write16_mock(Bus *bus, uint32_t addr, uint16_t value) {
    consume_write_cycle(ACTION_WRITE, 2, addr, value);
}
void bus_write32_mock(Bus *bus, uint32_t addr, uint32_t value) {
    consume_write_cycle(ACTION_WRITE, 4, addr, value);
}

uint8_t (*bus_read8)(Bus *bus, uint32_t addr) = bus_read8_mock;
uint16_t (*bus_read16)(Bus *bus, uint32_t addr) = bus_read16_mock;
uint32_t (*bus_read32)(Bus *bus, uint32_t addr) = bus_read32_mock;
uint32_t (*bus_fetch32)(Bus *bus, uint32_t addr) = bus_fetch32_mock;
void (*bus_write8)(Bus *bus, uint32_t addr, uint8_t value) = bus_write8_mock;
void (*bus_write16)(Bus *bus, uint32_t addr, uint16_t value) = bus_write16_mock;
void (*bus_write32)(Bus *bus, uint32_t addr, uint32_t value) = bus_write32_mock;

int main() {
    Cpu* cpu = create_cpu();
    State* initial = malloc(sizeof(State));
    State* final = malloc(sizeof(State));
    State* actual = malloc(sizeof(State));

    FILE* jsonl_file = fopen("../../tests/fixtures/r3000a/v1/ADD.jsonl", "r");
    if (!jsonl_file) {
        perror("Failed to open JSONL file");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    size_t line_count = 0;
    int read;

    while ((read = getline(&line, &len, jsonl_file)) != -1) {
        line_count++;
        struct json_object *parsed_json = json_tokener_parse(line);
        if (!parsed_json) {
            fprintf(stderr, "Failed to parse JSON on line %zu\n", line_count);
            continue;
        }

        struct json_object *name_json = json_object_object_get(parsed_json, "name");
        struct json_object *opcode_json = json_object_object_get(parsed_json, "opcode");
        struct json_object *initial_json = json_object_object_get(parsed_json, "initial");
        struct json_object *final_json = json_object_object_get(parsed_json, "final");
        struct json_object *cycles_json = json_object_object_get(parsed_json, "cycles");

        if (!name_json || !opcode_json || !initial_json || !final_json || !cycles_json) {
            fprintf(stderr, "Missing fields in JSON on line %zu\n", line_count);
            json_object_put(parsed_json);
            continue;
        }

        const char *name = json_object_get_string(name_json);
        int opcode = json_object_get_int(opcode_json);

        set_state(initial, initial_json);
        set_state(final, final_json);
        set_cycles(cycles, cycles_json);

        set_cpu_from_state(cpu, initial);

        cpu_step(cpu, NULL); // no actual bus needed since we're mocking the bus reads/writes

        set_state_from_cpu(actual, cpu);

        // Compare actual state with expected final state
        if (memcmp(actual, final, sizeof(State)) != 0) {
            fprintf(stderr, "Test failed for instruction %s (opcode: %u) on line %zu\n", name, opcode, line_count);
            // Find the first difference and print it
            if (cycle_index != cycle_count) {
                fprintf(stderr, "Not all cycles were consumed: expected %d, consumed %d\n", cycle_count, cycle_index);
            }
            for (int i = 0; i < 32; i++) {
                if (actual->R[i] != final->R[i]) {
                    fprintf(stderr, "Register R[%d] mismatch: expected 0x%08X, got 0x%08X\n", i, final->R[i], actual->R[i]);
                }
            }
            if (actual->hi != final->hi) {
                fprintf(stderr, "HI register mismatch: expected 0x%08X, got 0x%08X\n", final->hi, actual->hi);
            }
            if (actual->lo != final->lo) {
                fprintf(stderr, "LO register mismatch: expected 0x%08X, got 0x%08X\n", final->lo, actual->lo);
            }
            if (actual->EPC != final->EPC) {
                fprintf(stderr, "EPC mismatch: expected 0x%08X, got 0x%08X\n", final->EPC, actual->EPC);
            }
            if (actual->CAUSE != final->CAUSE) {
                fprintf(stderr, "CAUSE mismatch: expected 0x%08X, got 0x%08X\n", final->CAUSE, actual->CAUSE);
            }
            if (actual->PC != final->PC) {
                fprintf(stderr, "PC mismatch: expected 0x%08X, got 0x%08X\n", final->PC, actual->PC);
            }
            if (actual->delay.load_slot != final->delay.load_slot) {
                fprintf(stderr, "Delay slot mismatch: expected %d, got %d\n", final->delay.load_slot, actual->delay.load_slot);
            }
            if (actual->delay.load_take != final->delay.load_take) {
                fprintf(stderr, "Delay take mismatch: expected %d, got %d\n", final->delay.load_take, actual->delay.load_take);
            }
            if (actual->delay.load_target != final->delay.load_target) {
                fprintf(stderr, "Delay target mismatch: expected 0x%08X, got 0x%08X\n", final->delay.load_target, actual->delay.load_target);
            }
            if (actual->delay.branch_target != final->delay.branch_target) {
                fprintf(stderr, "Branch target mismatch: expected 0x%08X, got 0x%08X\n", final->delay.branch_target, actual->delay.branch_target);
            }
            if (actual->delay.branch_val != final->delay.branch_val) {
                fprintf(stderr, "Branch value mismatch: expected 0x%08X, got 0x%08X\n", final->delay.branch_val, actual->delay.branch_val);
            }
            assert(0 && "Test failed");
            break;
        } else {
            printf("Test passed for instruction %s (opcode: %u) on line %zu\n", name, opcode, line_count);
        }

        json_object_put(parsed_json);
    }

    free(line);
    fclose(jsonl_file);
    free(initial);
    free(final);
    free(actual);

    return 0;
}
