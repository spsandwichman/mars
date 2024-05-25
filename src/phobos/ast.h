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
        AstIdentTypePair var; /* should be IDENT_TYPE_PAIR*/ \
        Ast range; \
    })\
    AST_TYPE(IdentTypePair, IDENT_TYPE_PAIR, { \
        AstBase base; \
        Ast ident; \
        Ast type; /* can be null */ \
    })\
    AST_TYPE(EnumVariantItem, ENUM_VARIANT_ITEM, {\
        AstBase base; \
        Ast ident;\
        Ast value; /* can be null */ \
    })\
    \
    \
    \
    \
    \
    \
    AST_TYPE(BasicType, BASIC_TYPE, { \
        union { \
            AstBase base; \
            Token* lit; \
        }; \
    }) \
    AST_TYPE(StructType, STRUCT_TYPE, { \
            AstBase base; \
            da(Ast) fields; \
    }) \
    AST_TYPE(UnionType, UNION_TYPE, { \
            AstBase base; \
            da(Ast) fields; \
    }) \
    AST_TYPE(FnType, FN_TYPE, { \
            AstBase base; \
            da(Ast) parameters; \
            Ast block_symbol_override; /*this will be a string literal or NULL_STR if not set*/ \
            union { \
                da(Ast) returns; \
                Ast single_return; \
            }; \
            bool simple_return : 1; \
    }) \
    AST_TYPE(enum_type_expr, "enum type", { \
            AstBase base; \
            Ast backing_type;\
            da(Ast) variants; \
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
        AstBase * base;
#define AST_TYPE(ident, enumident, structdef) struct Ast##ident * ident;
        AST_NODES
#undef AST_TYPE
    };
    ast_type type;
} Ast;

da_typedef(Ast);

// generate Ast node typedefs
#define AST_TYPE(ident, enumident, structdef) typedef struct Ast##ident structdef Ast##ident;
    AST_NODES
#undef AST_TYPE

#define NULL_AST ((Ast){0})
#define is_null_AST(node) ((node).type == 0 || (node).rawptr == NULL)

extern char* ast_type_str[];
extern const size_t ast_type_size[];

Ast new_ast_node(arena* alloca, ast_type type);
void dump_tree(Ast node, int n);