#include "asmprinter.h"

void debugAsmPrinter(AsmModule* am) {
    //printf("func_len: %d\n", am->functions_len);
    for_urange(i, 0, am->functions_len) {
        AsmFunction* curr_func = am->functions[i];
        printf("Function: " str_fmt "\n", str_arg(curr_func->sym->name));
        for_urange(j, 0, curr_func->num_blocks) {
            AsmBlock* curr_block = curr_func->blocks[i];
            printf(str_fmt ":\n", str_arg(curr_block->label));
            for_urange(k, 0, curr_block->len) {
                AsmInst* curr_inst = curr_block->at[k];
                printf(str_fmt"\n", str_arg(curr_inst->template->asm_string));
            }
        }
    }
}