#include "exactval.h"
#include "arena.h"

// TODO rework exactvals to use a type* instead of a kind tag.
// it sucks to keep the 'ExactValue's and the 'checked_expr's synced up

ExactValue* alloc_exact_value(int aggregate_len, Arena* alloca) {
    ExactValue* ev;
    if (alloca == NULL) return NULL;
    if (alloca == USE_MALLOC) {
        ev = malloc(sizeof(ExactValue));
        ev->freeable = true;
    } else {
        ev = arena_alloc(alloca, sizeof(ExactValue), alignof(ExactValue));
    }
    memset(ev, 0, sizeof(*ev));
    if (aggregate_len != NO_AGGREGATE) {
        ev->as_aggregate.len = aggregate_len;
        if (alloca == USE_MALLOC)
            ev->as_aggregate.vals = malloc(sizeof(ExactValue*) * aggregate_len);
        else
            ev->as_aggregate.vals = arena_alloc(alloca, sizeof(ExactValue*) * aggregate_len, alignof(ExactValue*));
        memset(ev->as_aggregate.vals, 0, sizeof(ExactValue*) * aggregate_len);
    }
    return ev;
}

void destroy_exact_value(ExactValue* ev) {
    if (ev == NULL) return;
    if (!ev->freeable) return;
    free(ev->as_aggregate.vals);
    free(ev);
}

ExactValue* copy_ev_to_permanent(ExactValue* ev) {
    ExactValue* new = malloc(sizeof(ExactValue));
    memcpy(new, ev, sizeof(ExactValue));
    new->freeable = true;
    return new;
}