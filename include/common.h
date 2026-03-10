#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#define UNREACHABLE\
    do {\
        fprintf(stderr, "UNREACHABLE reached at %s:%d in %s()\n", __FILE__, __LINE__, __func__);\
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


// NOTE: this hash table implementation does not support removal (yet?), as the symbol table
// does not require removal

#define FNV_PRIME 0x100000001b3
#define FNV_BASIS 0xcbf29ce484222325

static uint64_t fnv1a_hash(const char *str) {
    uint64_t hash = FNV_BASIS;

    for (int i = 0; str[i] != '\0'; i++) {
        hash ^= (uint8_t)str[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

typedef struct {
    char **keys;
    void **items;
    size_t count;
    size_t capacity;
} HashTable;

void ht_insert(HashTable *ht, const char *str, void *item);
void ht_resize(HashTable *ht);
void *ht_search(HashTable *ht, const char *str);

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
void arena_clear(Arena *a);
void arena_free(Arena *a);
#define arena_alloc_type(arena_ptr, type) \
    (type *)arena_alloc((arena_ptr), sizeof(type))

#endif
