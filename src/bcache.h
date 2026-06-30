#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "bus.h"
#include "common.h"

#define BLOCK_SIZE 16 // Number of instructions in a basic block

typedef struct {
    bool valid; // Indicates if the block is valid

    uint32_t start_pc; // Starting program counter of the basic block
    uint32_t end_pc; // Ending program counter of the basic block (exclusive)

    uint32_t page0; // First page of the basic block
    uint32_t page0_gen; // Generation of the first page of the basic block

    uint32_t page1; // Second page of the basic block
    uint32_t page1_gen; // Generation of the second page of the basic block

    uint32_t instructions[BLOCK_SIZE]; // Instructions in the basic block
    uint8_t instruction_count; // Number of valid instructions in the block

} BasicBlock;

typedef struct {
    BasicBlock *block_page_table[BUS_PAGE_COUNT]; // Page table for basic blocks
    uint32_t block_generation[BUS_PAGE_COUNT]; // Generation numbers for each page
} BlockCache;

BlockCache *bcache_create();
void bcache_reset(BlockCache *cache);
void bcache_destroy(BlockCache *cache);

static inline void bcache_validate_block(BlockCache *cache, BasicBlock *block, uint32_t pc) {
    if (unlikely(!block || !block->valid)) {
        return; // Block is NULL or marked as invalid
    }

    if (block->start_pc != pc) {
        // Block does not start at the requested PC
        block->valid = false;
        return;
    }

    uint32_t page0 = block->page0;
    uint32_t page1 = block->page1;

    if (cache->block_generation[page0] != block->page0_gen) {
        // Block is invalid due to generation mismatch on page0
        block->valid = false;
        return;
    }
    if (page1 != page0 && cache->block_generation[page1] != block->page1_gen) {
        // Block is invalid due to generation mismatch on page1
        block->valid = false;
        return;
    }
}

static inline BasicBlock* bcache_get_block(BlockCache *cache, uint32_t pc) {
    uint32_t page = pc >> BUS_PAGE_SHIFT;
    BasicBlock *block = cache->block_page_table[page];
    bcache_validate_block(cache, block, pc);
    return block;
}

static inline uint32_t bcache_ensure_page_gen(BlockCache *cache, uint32_t page) {
    uint32_t page_gen = cache->block_generation[page];
    if (page_gen == 0) {
        page_gen = 1; // Start generation at 1
        cache->block_generation[page] = page_gen;
    }
    return page_gen;
}

static inline void bcache_set_block(BlockCache *cache, BasicBlock *block) {
    uint32_t page0 = get_page_index(block->start_pc);
    uint32_t page1 = get_page_index(block->end_pc);

    uint32_t page0_gen = cache->block_generation[page0];
    if (page0_gen == 0) {
        page0_gen = 1; // Start generation at 1
        cache->block_generation[page0] = page0_gen;
    }

    block->page0_gen = bcache_ensure_page_gen(cache, page0);
    cache->block_page_table[page0] = block;

    if (page1 != page0) {
        block->page1_gen = bcache_ensure_page_gen(cache, page1);
        cache->block_page_table[page1] = block;
    } else {
        block->page1_gen = block->page0_gen;
    }
}

static inline void bcache_invalidate_page_at_addr(BlockCache *cache, uint32_t addr) {
    uint32_t page = get_page_index(addr);
    uint32_t page_gen = cache->block_generation[page];
    if (page_gen == 0) {
        // no code has been generated
        return;
    }
    page_gen++;
    if (unlikely(page_gen == 0)) {
        page_gen = 1; // Avoid zero generation number
    }
    cache->block_generation[page] = page_gen;
}

#endif // BLOCK_H
