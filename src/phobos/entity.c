#include "entity.h"
#include "../deimos/deimos.h" //bodge

entity_table_list entity_tables;

entity_table* new_entity_table(entity_table* parent) {
    entity_table* et = malloc(sizeof(entity_table));
    *et = (entity_table){0};

    et->parent = parent;
    da_init(et, 1);

    et->alloca = arena_make(10*sizeof(Entity));
    
    da_append(&entity_tables, et);
    return et;
}

Entity* search_for_entity(entity_table* et, string ident) {
    if (et == NULL) return NULL;
    
    // for now, its linear search bc im too lazy to impl a hashmap
    for_urange(i, 0, et->len) {
        if (string_eq(et->at[i]->identifier, ident)) {
            return et->at[i];
        }
    }
    // if not found, search parent scope
    return search_for_entity(et->parent, ident);
}

Entity* new_entity(entity_table* et, string ident, Ast decl) {
    Entity* e = arena_alloc(&et->alloca, sizeof(Entity), alignof(Entity));
    *e = (Entity){0};
    e->identifier = ident;
    e->decl = decl;
    e->top = et;
    da_append(et, e);
    return e;
}