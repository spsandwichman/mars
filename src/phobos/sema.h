#pragma once
#define PHOBOS_CHECKER_H

#include "phobos.h"
#include "parse.h"
#include "term.h"
#include "ast.h"
#include "exactval.h"

#include "entity.h"
#include "type.h"

// emit an error that highlights an Ast node
#define error_at_node(module, node, msg, ...) do { \
    string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                 \
    mars_file* source_file = find_source_file((module), ast_str);                                 \
    if (source_file == NULL) CRASH("source file not found for Ast node");                         \
    error_at_string(source_file->path, source_file->src, ast_str, msg __VA_OPT__(,) __VA_ARGS__); \
} while (0)

// emit a warning that highlights an Ast node
#define warning_at_node(module, node, msg, ...) do { \
    string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                   \
    mars_file* source_file = find_source_file((module), ast_str);                                   \
    if (source_file == NULL) CRASH("source file not found for Ast node");                           \
    warning_at_string(source_file->path, source_file->src, ast_str, msg __VA_OPT__(,) __VA_ARGS__); \
} while (0)

void check_module_and_dependencies(MarsModule* mod);
void check_module(MarsModule* mod);
void collect_globals(MarsModule* mod, EntityTable* et);
void collect_entites(MarsModule* mod, EntityTable* et, da(Ast) stmts, bool global);

typedef struct checked_expr {
    Ast expr;

    type* type;
    ExactValue* ev; // for if is compile time constant
    bool use_returns : 1; // use return list of type* as the type. for functions that return multiple things

    bool mutable       : 1;
    bool addressable   : 1;
    bool local_ref     : 1; // so that we can warn against returning local pointers and shit
    bool local_derived : 1;
} checked_expr;

void check_stmt(MarsModule* mod, EntityTable* et, ast_func_literal_expr* fn, Ast stmt, bool global);
void check_decl_stmt(MarsModule* mod, EntityTable* et, ast_func_literal_expr* fn, Ast stmt, bool global);
void check_return_stmt(MarsModule* mod, EntityTable* et, ast_func_literal_expr* fn, Ast stmt);

// pass in a checked_expr struct for check_expr to fill out
void check_expr          (MarsModule* mod, EntityTable* et, Ast expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_ident_expr    (MarsModule* mod, EntityTable* et, Ast expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_literal_expr  (MarsModule* mod, EntityTable* et, Ast expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_unary_op_expr (MarsModule* mod, EntityTable* et, Ast expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_binary_op_expr(MarsModule* mod, EntityTable* et, Ast expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_cast_expr     (MarsModule* mod, EntityTable* et, Ast expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_fn_literal_expr(MarsModule* mod, EntityTable* et, Ast expr, checked_expr* info, bool must_comptime_const, type* typehint);

type* type_from_expr(MarsModule* mod, EntityTable* et, Ast expr, bool no_error, bool top);