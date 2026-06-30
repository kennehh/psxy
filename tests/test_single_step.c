#define _GNU_SOURCE

#include "cpu.h"
#include "psx.h"
#include "exceptions.h"
#include "single_step_bus.h"
#include <stdio.h>
#include <assert.h>
#include <json-c/json.h>
#include <stdbool.h>
#include <string.h>
// #include <dirent.h>

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

Cpu* cpu;
PSX psx;
State* initial;
State* final;
State* actual;

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
                state->R[i] = (uint32_t)json_object_get_uint64(reg_value);
            }
        }
    }

    struct json_object *hi_value = json_object_object_get(json_state, "hi");
    if (hi_value) {
        state->hi = (uint32_t)json_object_get_uint64(hi_value);
    }

    struct json_object *lo_value = json_object_object_get(json_state, "lo");
    if (lo_value) {
        state->lo = (uint32_t)json_object_get_uint64(lo_value);
    }

    struct json_object *epc_value = json_object_object_get(json_state, "EPC");
    if (epc_value) {
        state->EPC = (uint32_t)json_object_get_uint64(epc_value);
    }

    struct json_object *cause_value = json_object_object_get(json_state, "CAUSE");
    if (cause_value) {
        state->CAUSE = (uint32_t)json_object_get_uint64(cause_value);
    }

    struct json_object *pc_value = json_object_object_get(json_state, "PC");
    if (pc_value) {
        state->PC = (uint32_t)json_object_get_uint64(pc_value);
    }

    struct json_object *delay_obj = json_object_object_get(json_state, "delay");
    if (delay_obj && json_object_get_type(delay_obj) == json_type_object) {
        struct json_object *load_value = json_object_object_get(delay_obj, "load");
        if (load_value) {
            state->delay.load_slot = json_object_get_boolean(json_object_object_get(load_value, "slot"));
            state->delay.load_take = json_object_get_boolean(json_object_object_get(load_value, "take"));
            state->delay.load_target = (uint32_t)json_object_get_uint64(json_object_object_get(load_value, "target"));
        }
        struct json_object *branch_value = json_object_object_get(delay_obj, "branch");
        if (branch_value) {
            state->delay.branch_target = (uint32_t)json_object_get_uint64(json_object_object_get(branch_value, "target"));
            state->delay.branch_val = (uint32_t)json_object_get_uint64(json_object_object_get(branch_value, "val"));
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

    state->delay.load_slot = cpu->branch_state & BRANCH_STATE_IN_DELAY_SLOT;
    state->delay.load_take = cpu->branch_state == BRANCH_STATE_TAKEN;
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
    cpu->next_exc_code = EXC_NONE;

    cpu->branch_state = state->delay.load_slot | (state->delay.load_take << 1);
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

int test_file(const char *filename, Cpu *cpu) {
    FILE* jsonl_file = fopen(filename, "r");
    if (!jsonl_file) {
        perror("Failed to open JSONL file");
        return 1;
    }

    char line[4096];
    size_t len = 0;
    size_t line_count = 0;
    int read;

    while (fgets(line, sizeof(line), jsonl_file)) {
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

        cpu_step(&psx); // no actual bus needed since we're mocking the bus reads/writes

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
            exit(EXIT_FAILURE);
            break;
        }

        json_object_put(parsed_json);
    }

    printf("All tests passed for file: %s\n", filename);
    fclose(jsonl_file);
}


int main(int argc, char *argv[]) {
    cpu = cpu_create();
    memset(&psx, 0, sizeof(psx));
    psx.cpu = cpu;
    initial = malloc(sizeof(State));
    final = malloc(sizeof(State));
    actual = malloc(sizeof(State));

    // arg should be the path to the JSONL file
    if (argc < 2 || argc > 2) {
        fprintf(stderr, "Usage: %s <path_to_jsonl_file>\n", argv[0]);
        return 1;
    }

    test_file(argv[1], cpu);

    free(initial);
    free(final);
    free(actual);
    cpu_destroy(cpu);

    return 0;
}
