#ifndef Z_CONTAINERS_H
#define Z_CONTAINERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Z_VECTOR_DEFAULT_CAPACITY 4

typedef struct {
    void *items;
    size_t capacity;
    size_t count;
    size_t item_size;
} z_vector;

z_vector z_vector_create(size_t item_size);
void z_vector_free(z_vector *v);
void *z_vector_push(z_vector *v, const void *item);
void *z_vector_get(z_vector *v, size_t index);
void z_vector_set(z_vector *v, size_t index, const void *item);
void z_vector_pop(z_vector *v, void *out);
size_t z_vector_count(z_vector *v);

typedef struct {
    int next;
    int head;
} z_map_bucket;

typedef struct {
    void *key;
    void *value;
    int next;
} z_map_entry;

typedef struct {
    z_map_entry *entries;
    z_map_bucket *buckets;
    size_t capacity;
    size_t count;
    size_t key_size;
    size_t value_size;
} z_map;

typedef struct {
    z_map *map;
    size_t bucket_idx;
    int entry_idx;
} z_map_iter;

z_map *z_map_create(size_t key_size, size_t value_size);
void z_map_free(z_map *m);
void z_map_set(z_map *m, const void *key, const void *value);
void *z_map_get(z_map *m, const void *key);
bool z_map_has(z_map *m, const void *key);
void z_map_delete(z_map *m, const void *key);
size_t z_map_count(z_map *m);
z_map_iter z_map_iterate(z_map *m);
void *z_map_next(z_map *m, z_map_iter *iter);

#ifdef Z_CONTAINERS_IMPLEMENTATION

static size_t z_vector_grow_capacity(size_t cap) {
    return cap == 0 ? Z_VECTOR_DEFAULT_CAPACITY : cap * 2;
}

z_vector z_vector_create(size_t item_size) {
    z_vector v = {0};
    v.item_size = item_size;
    v.capacity = Z_VECTOR_DEFAULT_CAPACITY;
    v.items = malloc(v.capacity * item_size);
    return v;
}

void z_vector_free(z_vector *v) {
    if (v->items) {
        free(v->items);
        v->items = NULL;
    }
    v->capacity = 0;
    v->count = 0;
    v->item_size = 0;
}

void *z_vector_push(z_vector *v, const void *item) {
    if (v->count >= v->capacity) {
        v->capacity = z_vector_grow_capacity(v->capacity);
        v->items = realloc(v->items, v->capacity * v->item_size);
    }
    char *dst = (char *)v->items + v->count * v->item_size;
    memcpy(dst, item, v->item_size);
    v->count++;
    return dst;
}

void *z_vector_get(z_vector *v, size_t index) {
    if (index >= v->count) return NULL;
    return (char *)v->items + index * v->item_size;
}

void z_vector_set(z_vector *v, size_t index, const void *item) {
    if (index >= v->count) return;
    char *dst = (char *)v->items + index * v->item_size;
    memcpy(dst, item, v->item_size);
}

void z_vector_pop(z_vector *v, void *out) {
    if (v->count == 0) return;
    v->count--;
    if (out) {
        memcpy(out, (char *)v->items + v->count * v->item_size, v->item_size);
    }
}

size_t z_vector_count(z_vector *v) {
    return v->count;
}

static size_t z_map_hash(z_map *m, const void *key) {
    const uint8_t *bytes = (const uint8_t *)key;
    size_t h = 5381;
    for (size_t i = 0; i < m->key_size; i++) {
        h = ((h << 5) + h) + bytes[i];
    }
    return h;
}

static int z_map_find(z_map *m, const void *key) {
    if (m->count == 0) return -1;
    size_t h = z_map_hash(m, key) % m->capacity;
    for (int i = m->buckets[h].head; i >= 0; i = m->entries[i].next) {
        if (m->entries[i].key && memcmp(m->entries[i].key, key, m->key_size) == 0) {
            return i;
        }
    }
    return -1;
}

z_map *z_map_create(size_t key_size, size_t value_size) {
    z_map *m = (z_map *)malloc(sizeof(z_map));
    m->key_size = key_size;
    m->value_size = value_size;
    m->capacity = 8;
    m->count = 0;
    m->entries = (z_map_entry *)calloc(m->capacity, sizeof(z_map_entry));
    m->buckets = (z_map_bucket *)calloc(m->capacity, sizeof(z_map_bucket));
    for (size_t i = 0; i < m->capacity; i++) {
        m->buckets[i].head = -1;
        m->buckets[i].next = -1;
    }
    return m;
}

void z_map_free(z_map *m) {
    if (!m) return;
    for (size_t i = 0; i < m->count; i++) {
        if (m->entries[i].key) {
            free(m->entries[i].key);
            m->entries[i].key = NULL;
        }
        if (m->entries[i].value) {
            free(m->entries[i].value);
            m->entries[i].value = NULL;
        }
    }
    free(m->entries);
    free(m->buckets);
    free(m);
}

static void z_map_grow(z_map *m) {
    size_t old_cap = m->capacity;
    m->capacity *= 2;
    m->entries = (z_map_entry *)realloc(m->entries, m->capacity * sizeof(z_map_entry));
    m->buckets = (z_map_bucket *)realloc(m->buckets, m->capacity * sizeof(z_map_bucket));
    for (size_t i = old_cap; i < m->capacity; i++) {
        m->buckets[i].head = -1;
        m->buckets[i].next = -1;
    }
    for (size_t i = 0; i < m->count; i++) {
        if (m->entries[i].key) {
            size_t h = z_map_hash(m, m->entries[i].key) % m->capacity;
            m->entries[i].next = m->buckets[h].head;
            m->buckets[h].head = (int)i;
        }
    }
}

void z_map_set(z_map *m, const void *key, const void *value) {
    int idx = z_map_find(m, key);
    if (idx >= 0) {
        if (m->entries[idx].value) free(m->entries[idx].value);
        m->entries[idx].value = malloc(m->value_size);
        memcpy(m->entries[idx].value, value, m->value_size);
        return;
    }
    if (m->count > 0 && (float)m->count / m->capacity > 0.75f) z_map_grow(m);
    idx = (int)m->count;
    m->entries[idx].key = malloc(m->key_size);
    memcpy(m->entries[idx].key, key, m->key_size);
    m->entries[idx].value = malloc(m->value_size);
    memcpy(m->entries[idx].value, value, m->value_size);
    size_t h = z_map_hash(m, key) % m->capacity;
    m->entries[idx].next = m->buckets[h].head;
    m->buckets[h].head = (int)idx;
    m->count++;
}

void *z_map_get(z_map *m, const void *key) {
    int idx = z_map_find(m, key);
    if (idx >= 0) return m->entries[idx].value;
    return NULL;
}

bool z_map_has(z_map *m, const void *key) {
    return z_map_find(m, key) >= 0;
}

void z_map_delete(z_map *m, const void *key) {
    int idx = z_map_find(m, key);
    if (idx < 0) return;
    size_t h = z_map_hash(m, key) % m->capacity;
    m->buckets[h].head = m->entries[idx].next;
    free(m->entries[idx].key);
    m->entries[idx].key = NULL;
    free(m->entries[idx].value);
    m->entries[idx].value = NULL;
    m->count--;
}

size_t z_map_count(z_map *m) {
    return m->count;
}

z_map_iter z_map_iterate(z_map *m) {
    z_map_iter it = {m, 0, -1};
    if (m->count == 0) return it;
    for (size_t i = 0; i < m->capacity; i++) {
        if (m->buckets[i].head >= 0) {
            it.bucket_idx = i;
            it.entry_idx = m->buckets[i].head;
            break;
        }
    }
    return it;
}

void *z_map_next(z_map *m, z_map_iter *it) {
    (void)m;
    if (!it || it->entry_idx < 0) return NULL;
    void *value = m->entries[it->entry_idx].value;
    it->entry_idx = m->entries[it->entry_idx].next;
    if (it->entry_idx < 0) {
        it->bucket_idx++;
        for (; it->bucket_idx < m->capacity; it->bucket_idx++) {
            if (m->buckets[it->bucket_idx].head >= 0) {
                it->entry_idx = m->buckets[it->bucket_idx].head;
                break;
            }
        }
    }
    return value;
}

#endif

#ifdef __cplusplus
}
#endif

#endif