#pragma once
#define PHOBOS_ENTITY_H

#include "orbit.h"
#include "ast.h"
#include "parser.h"
#include "type.h"

typedef struct entity {
    string identifier;
    AST decl; // If it's NULL_AST, it hasn't been declared yet.

    type* entity_type;

    bool is_mutable : 1;
    bool is_type    : 1;

    bool visited : 1; // for cyclic dependency checking
} entity;

typedef struct entity_table_list {
    struct entity_table** at;
    size_t len;
    size_t cap;
} entity_table_list;

typedef struct entity_table {
    struct entity_table* parent;
    arena alloca;

    entity** at;
    size_t len;
    size_t cap;
} entity_table;

extern entity_table_list entity_tables;

u64 FNV_1a(string key); // for implementing a hash table later

entity_table* new_entity_table(entity_table* restrict parent);

entity* search_for_entity(entity_table* restrict et, string ident);
entity* new_entity(entity_table* restrict et, string ident, AST decl);
entity* new_type(entity_table* restrict et, string ident, AST decl);