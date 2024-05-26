#include "orbit.h"
#include "lex.h"
#include "term.h"
#include "arena.h"
#include "ast.h"
#include "parse.h"

const size_t ast_type_size[] = {
    0,
#define AST_TYPE(ident, identstr, structdef) sizeof(Ast##ident),
    AST_NODES
#undef AST_TYPE
    0
};

char* ast_type_str[] = {
    "invalid",
#define AST_TYPE(ident, enumident, structdef) #ident,
    AST_NODES
#undef AST_TYPE
    "COUNT",
};

// allocate and zero a new Ast node with an arena
Ast new_ast_node(Arena* alloca, ast_type type) {
    Ast node;
    void* node_ptr = arena_alloc(alloca, ast_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("internal: new_ast_node_p() could not allocate Ast node of type '%s' with size %d", ast_type_str[type], ast_type_size[type]);
    }
    memset(node_ptr, 0, ast_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}