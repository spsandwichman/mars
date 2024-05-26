#pragma once
#define PHOBOS_PARSER_H

#include "orbit.h"
#include "lex.h"
#include "ast.h"
#include "arena.h"

typedef struct Parser {
    Arena* alloca;

    da(Token) tokens;
    string path;
    string src;

    u64 current;

    Ast    module_decl;
    da(Ast) stmts;

    size_t num_nodes;

} Parser;

Parser make_parser_from_lexer(Lexer* l, Arena* alloca);
void parse_file(Parser* p);

#define new_ast_node_p(p, type) ((p)->num_nodes++, new_ast_node((p)->alloca, (type)))

Ast parse_module_decl(Parser* p);
Ast parse_stmt       (Parser* p);
Ast parse_block_stmt (Parser* p);
Ast parse_elif       (Parser* p);

Ast parse_binary_expr   (Parser* p, int precedence, bool no_tcl);
Ast parse_non_unary_expr(Parser* p, Ast lhs, int precedence, bool no_tcl);
Ast parse_unary_expr    (Parser* p, bool no_tcl);
Ast parse_atomic_expr   (Parser* p, bool no_tcl);

#define str_from_tokens(start, end) ((string){(start).raw, (end).raw - (start).raw + (end).len})

#define error_at_parser(p, message, ...) \
    error_at_string((p)->path, (p)->src, tok2str(current_token), \
    message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token_index(p, index, message, ...) \
    error_at_string((p)->path, (p)->src, (p)->tokens.at[index].text, \
    message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token(p, t, message, ...) \
    error_at_string((p)->path, (p)->src, tok2str((t)), \
    message __VA_OPT__(,) __VA_ARGS__)

#define error_at_AST(p, node, message, ...) \
    error_at_string((p)->path, (p)->src, str_from_tokens(*((node).base->start), *((node).base->end)), \
    message __VA_OPT__(,) __VA_ARGS__)

