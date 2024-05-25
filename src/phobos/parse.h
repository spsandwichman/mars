#pragma once
#define PHOBOS_PARSER_H

#include "orbit.h"
#include "lex.h"
#include "ast.h"
#include "arena.h"

typedef struct parser {
    Arena* alloca;
    da(Token) tokens;
    string path;
    string src;
    size_t current_tok;

    Ast    module_decl;
    da(Ast) stmts;

    size_t num_nodes;

} parser;

parser make_parser(Lexer* l, Arena* alloca);
void parse_file(parser* p);

#define new_ast_node_p(p, type) ((p)->num_nodes++, new_ast_node((p)->alloca, (type)))

Ast parse_module_decl(parser* p);
Ast parse_stmt       (parser* p);
Ast parse_block_stmt (parser* p);
Ast parse_elif       (parser* p);

Ast parse_binary_expr   (parser* p, int precedence, bool no_tcl);
Ast parse_non_unary_expr(parser* p, Ast lhs, int precedence, bool no_tcl);
Ast parse_unary_expr    (parser* p, bool no_tcl);
Ast parse_atomic_expr   (parser* p, bool no_tcl);

#define parse_type_expr(p) (parse_unary_expr((p), true))
#define parse_expr(p, no_cl) (parse_binary_expr(p, 0, no_cl))

#define current_token(p) ((p)->tokens.at[(p)->current_tok])
#define peek_token(p, n) ((p)->tokens.at[(p)->current_tok + (n)])
#define advance_token(p) (((p)->current_tok + 1 < (p)->tokens.len) ? ((p)->current_tok)++ : 0)
#define advance_n_tok(p, n) (((p)->current_tok + n < (p)->tokens.len) ? ((p)->current_tok)+=n : 0)

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

