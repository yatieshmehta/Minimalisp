#include "arena.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define ALIGN8(x) (((x) + 7) & ~7)

typedef struct arena_chunk {
    struct arena_chunk* next;
    size_t used;
    size_t capacity;
    char memory[];
} arena_chunk;

struct arena {
    arena_chunk* head;
    size_t chunk_size;
};

arena_t* arena_create(size_t chunk_size) {
    arena_t* arena = malloc(sizeof(arena_t));
    if (!arena) {
        perror("arena struct malloc");
        exit(1);
    }

    size_t total = sizeof(arena_chunk) + chunk_size;
    arena_chunk* chunk = malloc(total);
    if (!chunk) {
        perror("arena chunk malloc");
        exit(1);
    }

    chunk->next = NULL;
    chunk->used = 0;
    chunk->capacity = chunk_size;
    memset(chunk->memory, 0, chunk_size);

    arena->head = chunk;
    arena->chunk_size = chunk_size;

    return arena;
}

void* arena_alloc(arena_t* arena, size_t size) {
    size = ALIGN8(size);
    arena_chunk* chunk = arena->head;

    if (!chunk || ALIGN8(chunk->used) + size > chunk->capacity) {
        size_t total = sizeof(arena_chunk) + arena->chunk_size;
        arena_chunk* new_chunk = malloc(total);
        if (!new_chunk) {
            perror("arena_alloc: new chunk malloc");
            exit(1);
        }

        new_chunk->next = chunk;
        new_chunk->used = 0;
        new_chunk->capacity = arena->chunk_size;
        memset(new_chunk->memory, 0, arena->chunk_size);

        arena->head = new_chunk;
        chunk = new_chunk;
    }

    chunk->used = ALIGN8(chunk->used);
    void* ptr = chunk->memory + chunk->used;
    chunk->used += size;

    memset(ptr, 0, size);
    return ptr;
}

void arena_destroy(arena_t* arena) {
    arena_chunk* chunk = arena->head;
    while (chunk) {
        arena_chunk* next = chunk->next;
        free(chunk);
        chunk = next;
    }
    free(arena);
}
