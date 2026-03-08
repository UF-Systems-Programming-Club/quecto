#include "common.h"
#include <sys/_types/_null.h>

// for printing
const char *tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

void ht_resize(HashTable *ht) {
    size_t new_capacity = ht->capacity * 2;
    if (ht->capacity == 0) new_capacity = 256;
    char **new_keys = (char **)calloc(new_capacity, sizeof(char *));
    void **new_items = (void **)calloc(new_capacity, sizeof(void *));

    for (int i = 0; i < ht->capacity; i++) {
        if (ht->keys[i] == NULL) continue;
        char *str = ht->keys[i];
        void *item = ht->items[i];

        uint64_t hash = fnv1a_hash(str);
        size_t index = hash % new_capacity;
        while (new_keys[index] != NULL) {
            index = (index + 1) % new_capacity;
        }

        new_keys[index] = strdup(str);
        new_items[index] = item;
    }

    ht->keys = new_keys;
    ht->items = new_items;
    ht->capacity = new_capacity;
}

void ht_insert(HashTable *ht, const char *str, void *item) {
    if (ht->count * 2 >= ht->capacity) ht_resize(ht);

    uint64_t hash = fnv1a_hash(str);
    size_t index = hash % ht->capacity;
    while (ht->keys[index] != NULL) {
        if (strcmp(ht->keys[index], str) == 0) {
            ht->items[index] = item;
        }

        index = (index + 1) % ht->capacity;
    }

    ht->keys[index] = strdup(str);
    ht->items[index] = item;
    ht->count++;
}

void *ht_search(HashTable *ht, const char *str) {
    if (ht->capacity == 0) return NULL;

    uint64_t hash = fnv1a_hash(str);
    size_t index = hash % ht->capacity;
    while (ht->keys[index] != NULL) {
        if (strcmp(ht->keys[index], str) == 0) {
            return ht->items[index];
        }

        index = (index + 1) % ht->capacity;
    }

    return NULL;
}

void arena_create(Arena *a, size_t capacity) {
    a->capacity = capacity;
    a->size = 0;
    a->data = malloc(capacity);
    if (!a->data) {
        fprintf(stderr, "Failed to create arena of capacity %zu\n", capacity);
        exit(1);
    }
}

void *arena_alloc(Arena *a, size_t size) {
    // 8 byte alignment
    size_t alignment = sizeof(void *);
    size_t aligned_size = (size + (alignment - 1)) & ~(alignment - 1);

    if (a->size + aligned_size > a->capacity) {
        fprintf(stderr, "Arena overflow (capacity %zu)", a->capacity);
        exit(1);
    }

    void *ptr = a->data + a->size;
    a->size += aligned_size;

    // zero initialize
    memset(ptr, 0, aligned_size);
    return ptr;
}

void arena_reset(Arena *a) {
    a->size = 0;
}

void arena_free(Arena *a) {
    free(a->data);
    a->data = NULL;
    a->capacity = 0;
    a->size = 0;
}
