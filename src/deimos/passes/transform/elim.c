#include "deimos.h"

/* pass "elim" - remove instructions marked eliminated

    move non-IR_ELIMINATED instructions to the top of the basic block
    and set the length of the basic block to exclude the extra instructions.

*/

static void transcribe_w_no_elims(IR_BasicBlock* bb) {
    
    u64 place = 0;
    for_urange(i, 0, bb->len) {
        IR* ir = bb->at[i];
        if (ir == NULL || ir->tag == IR_ELIMINATED) continue;

        if (place == i) {
            place++;
            continue;
        }

        bb->at[place++] = ir;
    }
    bb->len = place;
}

IR_Module* ir_pass_elim(IR_Module* mod) {
    for_urange(i, 0, mod->functions_len) {
        for_urange(j, 0, mod->functions[i]->blocks.len) {
            IR_BasicBlock* bb = mod->functions[i]->blocks.at[j];
            transcribe_w_no_elims(bb);
        }
    }
    return mod;
}