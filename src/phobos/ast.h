#pragma once
#define PHOBOS_AST_H

#include "orbit.h"
#include "lex.h"
#include "arena.h"
#include "exactval.h"
#include "type.h"

typedef struct {
    Token* start;
    Token* end;
    // type* T; /* tentatively deleting this, it may be useful in the future but not now */
} AstBase;

// define all the Ast node macros
#define AST_NODES \
    AST_TYPE(IdentExpr, IDENT, { \
        union { \
            AstBase base; \
            Token* tok; \
        }; \
        struct entity* entity; \
        bool is_discard : 1; \
    }) \
    AST_TYPE(LitExpr, LIT_EXPR, { \
        AstBase base; \
        exact_value value; \
    }) \
    AST_TYPE(CompoundLitExpr, COMPOUND_LIT_EXPR, { \
        AstBase base; \
        Ast type; \
        da(Ast) elems; \
    }) \
    AST_TYPE(FunctionLitExpr, FUNCTION_LIT_EXPR, { \
        AstBase base; \
        Ast type; \
        Ast code_block; \
        \
        /* this is filled out by the checker */ \
        struct Entity** params;\
        struct Entity** returns;\
        u16 paramlen;\
        u16 returnlen;\
    }) \
    AST_TYPE(ParenExpr, PAREN_EXPR, { \
        AstBase base; \
        Ast subexpr; \
    }) \
    AST_TYPE(CastExpr, CAST_EXPR, { \
        AstBase base; \
        Ast type; \
        Ast rhs; \
        bool is_bitcast : 1; \
    }) \
    AST_TYPE(UnaryOpExpr, UNARY_OP_EXPR, { \
        AstBase base; \
        Token* op; \
        Ast inside; \
    }) \
    AST_TYPE(BinaryOpExpr, BINARY_OP_EXPR, { \
        AstBase base; \
        Token* op; \
        Ast lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(EntitySelectorExpr, ENTITY_SELECTOR_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(SelectorExpr, SELECTOR_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
        u32 field_index; /* filled out by checker */ \
    }) \
    AST_TYPE(ReturnSelectorExpr, RETURN_SELECTOR_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(ImplSelectorExpr, IMPL_SELECTOR_EXPR, { \
        AstBase base; \
        Ast rhs; \
    }) \
    AST_TYPE(IndexExpr, INDEX_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast inside; \
    }) \
    AST_TYPE(SliceExpr, SLICE_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast inside_left; \
        Ast inside_right; \
    }) \
    AST_TYPE(CallExpr, CALL_EXPR, { \
        AstBase base; \
        Ast lhs; \
        da(Ast) params; \
        bool force_inline; \
    }) \
    AST_TYPE(RangeExpr, RANGE_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
        bool inclusive; \
    }) \
    AST_TYPE(BasicForPattern, BASIC_FOR_PATTERN, { \
        AstBase base; \
        Ast prelude; \
        Ast condition; \
        Ast iteration; \
    })\
    AST_TYPE(RangedForPattern, RANGED_FOR_PATTERN, { \
        AstBase base; \
        Ast index; \
        Ast range; \
    })\
    \
    \
    AST_TYPE(module_decl, "module declaration", { \
        AstBase base; \
        Token* name; \
    }) \
    AST_TYPE(import_stmt, "import statement", { \
        AstBase base; \
        Ast name; \
        Ast path; \
        \
        string realpath; \
    }) \
    AST_TYPE(block_stmt, "statement block", { \
        AstBase base; \
        da(Ast) stmts; \
    }) \
    AST_TYPE(decl_stmt, "declaration", { \
        AstBase base; \
        da(Ast) lhs; \
        Ast rhs; \
        Ast type; \
        bool is_mut        : 1; \
    }) \
    AST_TYPE(type_decl_stmt, "type declaration", { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(assign_stmt, "assignment", { \
        AstBase base; \
        da(Ast) lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(comp_assign_stmt, "compound assignment", { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
        Token* op; \
    }) \
    AST_TYPE(if_stmt, "if statement", { \
        AstBase base; \
        Ast condition; \
        Ast if_branch; \
        Ast else_branch; \
        bool is_elif : 1; \
    }) \
    AST_TYPE(switch_stmt, "switch statement", { \
        AstBase base; \
        Ast expr; \
        da(Ast) cases; \
    }) \
    AST_TYPE(case, "case statement", { \
        AstBase base; \
        da(Ast) matches; \
        Ast block; \
    }) \
    AST_TYPE(while_stmt, "while loop", { \
        AstBase base; \
        Ast condition; \
        Ast block; \
    }) \
    AST_TYPE(for_stmt, "for loop", { \
        AstBase base; \
        Ast pattern; \
        Ast block; \
    }) \
    AST_TYPE(extern_stmt, "extern statement", { \
        AstBase base; \
        Ast decl; \
    }) \
    AST_TYPE(defer_stmt, "defer statement", { \
        AstBase base; \
        Ast stmt; \
    }) \
    AST_TYPE(expr_stmt, "expression statement", { \
        AstBase base; \
        Ast expression; \
    }) \
    AST_TYPE(return_stmt, "return statement", { \
        AstBase base; \
        da(Ast) returns; \
    }) \
    AST_TYPE(break_stmt, "break statement", { \
        AstBase base; \
        Ast label; \
    }) \
    AST_TYPE(continue_stmt, "continue statement", { \
        AstBase base; \
        Ast label; \
    }) \
    AST_TYPE(fallthrough_stmt, "fallthrough statement", { \
        AstBase base; \
    }) \
    AST_TYPE(empty_stmt, "empty statement", { \
        union{ \
        AstBase base; \
        Token* tok; \
        }; \
    }) \
    AST_TYPE(label_stmt, "label", { \
        AstBase base; \
        Ast label; \
    }) \
    \
    \
    \
    \
    AST_TYPE(basic_type_expr, "basic type literal", { \
        union { \
            AstBase base; \
            Token* lit; \
        }; \
    }) \
    AST_TYPE(struct_type_expr, "struct type", { \
            AstBase base; \
            da(AST_typed_field) fields; \
            bool smart_pack : 1;\
    }) \
    AST_TYPE(union_type_expr, "union type", { \
            AstBase base; \
            da(AST_typed_field) fields; \
    }) \
    AST_TYPE(fn_type_expr, "fn type", { \
            AstBase base; \
            da(AST_typed_field) parameters; \
            Ast block_symbol_override; /*this will be a string literal or NULL_STR if not set*/ \
            union { \
                da(AST_typed_field) returns; \
                Ast single_return; \
            }; \
            bool simple_return : 1; \
    }) \
    AST_TYPE(enum_type_expr, "enum type", { \
            AstBase base; \
            Ast backing_type;\
            da(AST_enum_variant) variants; \
    }) \
    AST_TYPE(array_type_expr, "array type", { \
            AstBase base; \
            Ast subexpr; \
            Ast length; \
    }) \
    AST_TYPE(slice_type_expr, "slice type", { \
            AstBase base; \
            Ast subexpr; \
            bool mutable : 1; \
    }) \
    AST_TYPE(pointer_type_expr, "pointer type", { \
            AstBase base; \
            Ast subexpr; \
            bool mutable : 1; \
    }) \
    AST_TYPE(distinct_type_expr, "distinct type", { \
            AstBase base; \
            Ast subexpr; \
    }) \


// generate the enum tags for the Ast tagged union
typedef u16 ast_type; enum {
    AST_invalid,
#define AST_TYPE(ident, enumident, structdef) AST_##ident,
    AST_NODES
#undef AST_TYPE
    AST_COUNT,
};

// generate tagged union Ast type
typedef struct Ast {
    union {
        void* rawptr;
        AstBase * restrict base;
#define AST_TYPE(ident, enumident, structdef) struct ast_##ident * restrict as_##ident;
        AST_NODES
#undef AST_TYPE
    };
    ast_type type;
} Ast;

typedef struct {
    Ast field;
    Ast type; // may be NULL_AST if the type is the same as the next field
} AST_typed_field;


typedef struct {
    Ast ident;
    i64 value;
} AST_enum_variant;

da_typedef(Ast);
da_typedef(AST_enum_variant);
da_typedef(AST_typed_field);

// generate Ast node typedefs
#define AST_TYPE(ident, enumident, structdef) typedef struct ast_##ident structdef ast_##ident;
    AST_NODES
#undef AST_TYPE

#define NULL_AST ((Ast){0})
#define is_null_AST(node) ((node).type == 0 || (node).rawptr == NULL)

extern char* ast_type_str[];
extern const size_t ast_type_size[];

Ast new_ast_node(arena* alloca, ast_type type);
void dump_tree(Ast node, int n);