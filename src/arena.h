#pragma once
#define ARENA_H

#include "orbit.h"

// memory arenas

typedef struct {
    void* raw;
    u32 offset;
    u32 size;
} ArenaBlock;

da_typedef(ArenaBlock);

typedef struct Arena {
    da(ArenaBlock) list;
    u32 arena_size;
} Arena;

ArenaBlock arena_block_make(size_t size);
void  arena_block_delete(ArenaBlock* a);
void* arena_block_alloc(ArenaBlock* a, size_t size, size_t align);

Arena arena_make(size_t size);
void  arena_delete(Arena* al);
void* arena_alloc(Arena* al, size_t size, size_t align);

size_t align_forward(size_t ptr, size_t align);