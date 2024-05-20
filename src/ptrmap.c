#include "ptrmap.h"

size_t hashfunc(void* key) {
    size_t hash = 5381;
    size_t k = (size_t) key;
    hash = ((hash << 5) + hash) + ((k >> 0)  & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 8)  & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 16) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 24) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 32) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 40) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 48) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 56) & 0xFF);
    return hash;
}

PtrMap* ptrmap_init(PtrMap* hm, size_t capacity) {
    hm->cap = capacity;
    hm->vals = mars_alloc(sizeof(hm->vals[0])*hm->cap);
    hm->keys = mars_alloc(sizeof(hm->keys[0])*hm->cap);
    return hm;
}

void ptrmap_destroy(PtrMap* hm) {
    if (hm->keys) mars_free(hm->keys);
    if (hm->vals) mars_free(hm->vals);
    *hm = (PtrMap){0};
}

void ptrmap_put(PtrMap* hm, void* key, void* val) {
    if (!key) return;
    size_t hash_index = hashfunc(key) % hm->cap;

    // mars_free slot
    if (hm->keys[hash_index] == NULL || hm->keys[hash_index] == key) {
        hm->keys[hash_index] = key;
        hm->vals[hash_index] = val;
        return;
    }

    // search for nearby mars_free slot
    for (size_t i = (hash_index + 1) % hm->cap; i != hash_index; i++) {
        if (i >= hm->cap) i = 0;
        if ((hm->keys[i] == NULL) || hm->keys[hash_index] == key) {
            hm->keys[i] = key;
            hm->vals[i] = val;
            return;
        }
    }

    // we gotta resize
    PtrMap new_hm;
    ptrmap_init(&new_hm, hm->cap*2);

    // copy all the old entries into the new hasptrmap
    for (size_t i = 0; i < hm->cap; i++) {
        if (hm->keys[i] == NULL) continue;
        ptrmap_put(&new_hm, hm->keys[i], hm->vals[i]);
    }
    ptrmap_put(&new_hm, key, val);

    // destroy old map
    mars_free(hm->keys);
    mars_free(hm->vals);
    *hm = new_hm;
}

void* ptrmap_get(PtrMap* hm, void* key) {
    if (!key) return PTRMAP_NOT_FOUND;
    size_t hash_index = hashfunc(key) % hm->cap;

    // key found in first slot
    if (hm->keys[hash_index] == key) {
        return hm->vals[hash_index];
    }

    // linear search next slots
    for (size_t i = hash_index + 1; i != hash_index; i++) {
        if (i >= hm->cap) i = 0;
        if (hm->keys[i] == NULL) return PTRMAP_NOT_FOUND;
        if (hm->keys[hash_index] == key) return hm->vals[i];
    }

    return PTRMAP_NOT_FOUND;
}

void ptrmap_remove(PtrMap* hm, void* key) {
    if (!key) return;

    size_t hash_index = hashfunc(key) % hm->cap;

    // key found in first slot
    if (hm->keys[hash_index] == key) {
        hm->keys[hash_index] = NULL;
        hm->vals[hash_index] = NULL;
        return;
    }

    // linear search next slots
    for (size_t i = hash_index + 1; i != hash_index; i++) {
        if (i >= hm->cap) i = 0;
        if (hm->keys[i] == NULL) return;
        if (hm->keys[hash_index] == key) {
            hm->keys[hash_index] = NULL;
            hm->vals[hash_index] = NULL;
            return;
        }
    }
}

void ptrmap_reset(PtrMap* hm) {
    memset(hm->vals, 0, sizeof(hm->vals[0])*hm->cap);
    memset(hm->keys, 0, sizeof(hm->keys[0])*hm->cap);
}