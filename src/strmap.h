#pragma once
#define STRMAP_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "orbit.h"

typedef struct StrMap {
    string* keys;
    void** vals;
    size_t cap; // capacity
} StrMap;

#define STRMAP_NOT_FOUND ((void*)0xDEAD)

size_t hashfunc(string str);

StrMap* strmap_init(StrMap* hm, size_t capacity);
void  strmap_reset(StrMap* hm);
void  strmap_destroy(StrMap* hm);
void  strmap_put(StrMap* hm, string key, void* val);
void  strmap_remove(StrMap* hm, string key);
void* strmap_get(StrMap* hm, string key);
