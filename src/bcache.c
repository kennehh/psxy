#include <stdio.h>
#include <stdlib.h>
#include "bcache.h"

BlockCache *bcache_create() {
    BlockCache *cache = (BlockCache *)malloc(sizeof(BlockCache));
    if (!cache) {
        fprintf(stderr, "Failed to allocate BlockCache structure\n");
        exit(EXIT_FAILURE);
    }
    bcache_reset(cache);
    return cache;
}

void bcache_reset(BlockCache *cache) {
    for (int i = 0; i < BUS_PAGE_COUNT; i++) {
        BasicBlock *block = cache->block_page_table[i];
        if (block) free(block);
        cache->block_page_table[i] = NULL;
        cache->block_generation[i] = 0;
    }
}

void bcache_destroy(BlockCache *cache) {
    if (!cache) return;

    for (int i = 0; i < BUS_PAGE_COUNT; i++) {
        BasicBlock *block = cache->block_page_table[i];
        if (block) free(block);
        cache->block_page_table[i] = NULL;
    }

    free(cache);
}
