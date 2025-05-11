#ifndef ARENA_H
#define ARENA_H

typedef struct arena arena_t;
extern arena_t* global_arena;
extern arena_t* temp_arena;


arena_t* arena_create(size_t size);
void* arena_alloc(arena_t* a, size_t size);
void arena_destroy(arena_t* a);

#endif
