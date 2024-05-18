#pragma once
#define PHOBOS_ENTITY_H

#include "orbit.h"
#include "phobos.h"
#include "ast.h"
#include "parse.h"
#include "type.h"

typedef struct Entity Entity;
typedef struct entity_table_list entity_table_list;
typedef struct entity_table entity_table;

typedef struct Entity {
    string identifier;
    AST decl; // If it's NULL_AST, it hasn't been declared yet.

    union {
        type* entity_type;
        mars_module* module;
    };

    exact_value* const_val;
    entity_table* top; // scope in which it is declared

    // filled out by checker
    struct IR* stackalloc;

    union {
        u16 param_idx;
        u16 return_idx;
    };

    bool is_param : 1;
    bool is_return : 1;

    bool is_const      : 1;
    bool is_mutable    : 1;
    bool is_type       : 1;
    bool is_module     : 1;
    bool is_extern     : 1;
    bool is_used       : 1;
    bool is_pointed_to : 1; // does its pointer ever get taken?

    bool checked : 1;
    bool visited : 1; // for cyclic dependency checking
} Entity;

typedef struct entity_table_list {
    entity_table** at;
    size_t len;
    size_t cap;
} entity_table_list;

typedef struct entity_table {
    entity_table* parent;
    arena alloca;

    Entity** at;
    size_t len;
    size_t cap;
} entity_table;

extern entity_table_list entity_tables;

u64 FNV_1a(string key); // for implementing a hash table later

entity_table* new_entity_table(entity_table* parent);

Entity* search_for_entity(entity_table* et, string ident);
Entity* new_entity(entity_table* et, string ident, AST decl);