#pragma once
#define PHOBOS_ENTITY_H

#include "orbit.h"
#include "phobos.h"
#include "ast.h"
#include "parse.h"
#include "type.h"

typedef struct Entity Entity;
typedef struct EntityTableList EntityTableList;
typedef struct EntityTable EntityTable;

typedef struct Entity {
    string identifier;
    Ast decl; // If it's NULL_AST, it hasn't been declared yet.

    union {
        type* entity_type;
        MarsModule* module;
    };

    ExactValue* const_val;
    EntityTable* top; // scope in which it is declared

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

    bool checked : 1;
    bool visited : 1; // for cyclic dependency checking
} Entity;

typedef struct EntityTableList {
    EntityTable** at;
    size_t len;
    size_t cap;
} EntityTableList;

typedef struct EntityTable {
    EntityTable* parent;
    Arena alloca;

    Entity** at;
    size_t len;
    size_t cap;
} EntityTable;

extern EntityTableList entity_tables;

EntityTable* new_entity_table(EntityTable* parent);

Entity* search_for_entity(EntityTable* et, string ident);
Entity* new_entity(EntityTable* et, string ident, Ast decl);