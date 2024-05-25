#pragma once
#define PHOBOS_H

#include "orbit.h"
#include "lex.h"
#include "parse.h"
#include "ast.h"

da_typedef(Lexer);
da_typedef(parser);

typedef struct {
    string path;
    string src;
} mars_file;

da_typedef(mars_file);

typedef struct {
    struct MarsModule ** at;
    size_t len;
    size_t cap;
} module_list;

typedef struct MarsModule {
    string module_name;
    string module_path;
    da(mars_file) files;
    module_list import_list;

    da(Ast) program_tree;
    Arena AST_alloca;
    Arena temp_alloca;

    bool visited : 1; // checking shit
    bool checked : 1; // has been FULLY CHECKED by the checker
} MarsModule;

typedef char* cstring;

da_typedef(cstring);


MarsModule* parse_module(string input_path);

// creates a compilation unit from a list of parsers.
// stitches the unchecked ASTs together and such
MarsModule* create_module(da(parser)* pl, Arena alloca);

// find the source file of a snippet of code
// NOTE: the snippet must be an actual substring (is_within() must return true) of one of the files
mars_file* find_source_file(MarsModule* cu, string snippet);