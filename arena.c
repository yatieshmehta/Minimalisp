#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct arena_chunk {
    struct arena_chunk* next;
    size_t used;
    size_t capacity;
    char memory[];
} arena_chunk;

typedef struct arena {
    arena_chunk* head;
    size_t chunk_size;
} arena_t;

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
    if (size > arena->chunk_size) {
        fprintf(stderr, "Arena allocation size %zu exceeds chunk size %zu\n", size, arena->chunk_size);
        exit(1);
    }

    arena_chunk* chunk = arena->head;

    if (!chunk || chunk->used + size > chunk->capacity) {
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

    void* ptr = chunk->memory + chunk->used;
    memset(ptr, 0, size);
    chunk->used += size;
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
