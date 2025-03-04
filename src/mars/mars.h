#pragma once
#define MARS_H

#include "common/orbit.h"
#include "common/alloc.h"
#include "mars/term.h"

#define DEFAULT_TARGET str("x86_64-linux-asm")

typedef struct cmd_arg_s {
    string key;
    string val;
} cmd_arg;

typedef struct flag_set_s {
    string input_path;
    string output_path;
    bool output_dot;
    bool print_timings;
    bool dump_AST;

    int target_arch;
    int target_system;
    int target_product;
} flag_set;

cmd_arg make_argument(char* s);
void load_arguments(int argc, char* argv[], flag_set* fl);

void print_help();

extern flag_set mars_flags;

#define MARS_PASS(module, name) \
    fe_sched_module_pass((module), &(name));

#define MARS_STANDARD_PASSES(module)         \
    do {                                     \
        MARS_PASS(module, fe_pass_verify)    \
        MARS_PASS(module, fe_pass_stackprom) \
        MARS_PASS(module, fe_pass_tdce)      \
    } while (0)
