#include "common.h"

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
