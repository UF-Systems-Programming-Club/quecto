#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#define MAX_PARAMS 4

#define UNREACHABLE(err)\
    do {\
        fprintf(stderr, "UNREACHABLE reached at %s:%d in %s(): %s\n", __FILE__, __LINE__, __func__, (err));\
        abort();\
    } while (0)

/* Struct definition needed to use array macros.
 * typedef struct {
 *     T items;
 *     size_t count;
 *     size_t capacity;
 * } array;
*/

#define array_append(array, item)\
    do {\
        if ((array).count >= (array).capacity) {\
            if ((array).capacity == 0) (array).capacity = 128;\
            (array).capacity *= 2;\
            (array).items = realloc((array).items, (array).capacity * sizeof(*(array).items));\
        }\
        (array).items[(array).count++] = item;\
    } while (0)

#define array_init(array, size)\
    do {\
        (array).capacity = size;\
        (array).items = malloc((array).capacity * sizeof(*(array).items));\
        (array).count = 0;\
    } while (0)

#define array_free(array)\
    do {\
        (array).capacity = 0;\
        free((array).items);\
        (array).count = 0;\
    } while (0)

// TODO: make sure this minimum isn't too small. don't want to be too memory wasteful
// and most arena arrays will be a small number of ast children
#define ARENA_ARRAY_MINIMUM_CAP 16
#define arena_array_append(arena, array, item)\
    do {\
        if ((array).count >= (array).capacity) {\
            size_t old_capacity = (array).capacity;\
            if ((array).capacity == 0) (array).capacity = ARENA_ARRAY_MINIMUM_CAP;\
            (array).capacity *= 2;\
            (array).items = arena_realloc(arena, (array).items, old_capacity * sizeof(*(array).items), (array).capacity * sizeof(*(array).items));\
        }\
        (array).items[(array).count++] = item;\
    } while (0)

// NOTE: this hash table implementation does not support removal (yet?), as the symbol table
// does not require removal

#define FNV_PRIME 0x100000001b3
#define FNV_BASIS 0xcbf29ce484222325

uint64_t fnv1a_hash(const void *str, size_t n);


typedef struct {
    size_t size;
    void *data;
} Key;

typedef struct {
    Key *keys;
    void **items;
    size_t count;
    size_t capacity;
} HashTable;

void ht_insert(HashTable *ht, const char *str, void *item);
void ht_ninsert(HashTable *ht, const void *key, size_t key_size, void *item);
void ht_resize(HashTable *ht);
void *ht_search(HashTable *ht, const char *str);
void *ht_nsearch(HashTable *ht, const void *key, size_t key_size);

// for printing

extern const char* tabs;

#define print_indent(extra, fmt, ...) \
    printf("%.*s" fmt, indent +(extra), tabs, ##__VA_ARGS__);

// currently only a fixed sized arena
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} Arena;

void arena_create(Arena *a, size_t capacity);
void *arena_alloc(Arena *a, size_t size);
void *arena_realloc(Arena *a, void *ptr, size_t old_size, size_t new_size);
void *arena_intern(Arena *a, HashTable *intern_table, const void *value, size_t size);
void arena_clear(Arena *a);
void arena_free(Arena *a);
#define arena_alloc_type(arena_ptr, type) \
    (type *)arena_alloc((arena_ptr), sizeof(type))

typedef struct {
    size_t len;
    const char *str;
} StringView;

#define sv_literal(str) (StringView){ sizeof(str) - 1, (str) }
#define sv_fmt "%.*s"
#define sv_arg(sv) (int)(sv).len, (sv).str

#endif
