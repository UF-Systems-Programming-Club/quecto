#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

/* Struct definition needed to use array macros
 * typedef struct {
 *     T items;
 *     size_t count;
 *     size_t capacity;
 * } array;
*/

#define array_append(array, item)\
    do {\
        if (array.count >= array.capacity) {\
            if (array.capacity == 0) array.capacity = 128;\
            array.capacity *= 2;\
            array.items = realloc(array.items, array.capacity * sizeof(*array.items));\
        }\
        array.items[array.count++] = item;\
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

#endif
