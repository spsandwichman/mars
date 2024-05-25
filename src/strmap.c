#include "StrMap.h"

// FNV_1a
u64 hashfunc(string key) {
    const u64 FNV_OFFSET = 14695981039346656037ull;
    const u64 FNV_PRIME  = 1099511628211ull;

    u64 hash = FNV_OFFSET;
    for_urange(i, 0, key.len) {
        hash ^= (u64)(u8)(key.raw[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}

StrMap* strmap_init(StrMap* sm, size_t capacity) {
    sm->cap = capacity;
    sm->vals = malloc(sizeof(sm->vals[0])*sm->cap);
    sm->keys = malloc(sizeof(sm->keys[0])*sm->cap);
    memset(sm->vals, 0, sizeof(sm->vals[0])*sm->cap);
    memset(sm->keys, 0, sizeof(sm->keys[0])*sm->cap);
    return sm;
}

void strmap_destroy(StrMap* sm) {
    if (sm->keys) free(sm->keys);
    if (sm->vals) free(sm->vals);
    *sm = (StrMap){0};
}

void strmap_put(StrMap* sm, string key, void* val) {
    if (is_null_str(key)) return;
    size_t hash_index = hashfunc(key) % sm->cap;

    // free slot
    if (sm->keys[hash_index].raw == NULL || string_eq(sm->keys[hash_index], key)) {
        sm->keys[hash_index] = key;
        sm->vals[hash_index] = val;
        return;
    }

    // search for nearby free slot
    for (size_t i = (hash_index + 1) % sm->cap; i != hash_index; i++) {
        if (i >= sm->cap) i = 0;
        if ((sm->keys[i] == NULL) || (strcmp(sm->keys[i], key) == 0)) {
            sm->keys[i] = key;
            sm->vals[i] = val;
            return;
        }
    }

    // we gotta resize
    StrMap new_sm;
    strmap_init(&new_sm, sm->cap*2);

    // copy all the old entries into the new hasstrmap
    for (size_t i = 0; i < sm->cap; i++) {
        if (sm->keys[i].raw == NULL) continue;
        strmap_put(&new_sm, sm->keys[i], sm->vals[i]);
    }
    strmap_put(&new_sm, key, val);

    // destroy old map
    free(sm->keys);
    free(sm->vals);
    *sm = new_sm;
}

void* strmap_get(StrMap* sm, string key) {
    if (!key.raw) return STRMAP_NOT_FOUND;
    size_t hash_index = hashfunc(key) % sm->cap;

    // key found in first slot
    if (strcmp(sm->keys[hash_index], key) == 0) {
        return sm->vals[hash_index];
    }

    // linear search next slots
    for (size_t i = hash_index + 1; i != hash_index; i++) {
        if (i >= sm->cap) i = 0;
        if (sm->keys[i] == NULL) return STRMAP_NOT_FOUND;
        if (strcmp(sm->keys[i], key) == 0) return sm->vals[i];
    }

    return STRMAP_NOT_FOUND;
}

void strmap_remove(StrMap* sm, string key) {
    if (!key) return;
    size_t hash_index = hashfunc(key) % sm->cap;

    // key found in first slot
    if (strcmp(sm->keys[hash_index], key) == 0) {
        sm->keys[hash_index] = NULL;
        sm->vals[hash_index] = NULL;
        return;
    }

    // linear search next slots
    for (size_t i = hash_index + 1; i != hash_index; i++) {
        if (i >= sm->cap) i = 0;
        if (sm->keys[i] == NULL) return;
        if (strcmp(sm->keys[i], key) == 0) {
            sm->keys[hash_index] = NULL;
            sm->vals[hash_index] = NULL;
            return;
        }
    }
}

void strmap_reset(StrMap* sm) {
    memset(sm->vals, 0, sizeof(sm->vals[0])*sm->cap);
    memset(sm->keys, 0, sizeof(sm->keys[0])*sm->cap);
}