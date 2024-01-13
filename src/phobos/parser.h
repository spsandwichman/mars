#pragma once
#define PHOBOS_PARSER_H

#include "orbit.h"
#include "lexer.h"
#include "ast.h"
#include "arena.h"

typedef struct parser {
    arena alloca;
    dynarr(token) tokens;
    string path;
    string src;
    size_t current_tok;

    AST    module_decl;
    AST    head;
} parser;

parser make_parser(lexer* restrict l, arena alloca);
void parse_file(parser* restrict p);

AST parse_module_decl(parser* restrict p);
AST parse_stmt(parser* restrict p, bool expect_semicolon);
AST parse_block_stmt(parser* restrict p);
AST parse_elif(parser* restrict p);