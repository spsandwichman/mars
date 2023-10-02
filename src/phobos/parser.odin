package phobos

import "core:fmt"

parser :: struct {
    file : ^file,
    lex  : ^lexer,

    curr_tok_index : int,

    curr_scope : ^scope_meta,

    node_stack      : [dynamic]AST,
    directive_stack : [dynamic]AST,
}

new_parser :: proc(f: ^file, l: ^lexer) -> (p: ^parser) {
    p = new(parser)
    p.file = f
    p.lex = l

    p.directive_stack = make([dynamic]AST)

    return
}

current_token :: proc(p: ^parser) -> ^lexer_token {
    return &(p.lex.buffer[p.curr_tok_index])
}

peek_token :: proc(p: ^parser, offset : int = 1) -> ^lexer_token {
    return &(p.lex.buffer[p.curr_tok_index + offset])
}

peek_until :: proc(p: ^parser, kind: token_kind) -> ^lexer_token {
    offset := 0
    for (p.lex.buffer[p.curr_tok_index + offset]).kind != kind {
        offset += 1
    }
    return &(p.lex.buffer[p.curr_tok_index + offset])
}

advance_token :: proc(p: ^parser, offset : int = 1) -> ^lexer_token {
    p.curr_tok_index += offset
    return &(p.lex.buffer[p.curr_tok_index])
}

advance_until :: proc(p: ^parser, kind: token_kind) -> ^lexer_token {
    for p.lex.buffer[p.curr_tok_index].kind != kind {
        advance_token(p)
    }
    return &(p.lex.buffer[p.curr_tok_index])
}

parse_file :: proc(p: ^parser) {

    append(&p.file.stmts, parse_module_decl(p))

    //TODO("fucking everything")
    for {
        //parse_stmt(p)
        break
    }

}

parse_module_decl :: proc(p: ^parser) -> (node: AST) {

    module_declaration := new_module_decl_stmt(nil,nil)

    module_declaration.start = current_token(p)

    // module name;
    // ^~~~~^
    if current_token(p).kind != .keyword_module {
        error(p.file.path, p.lex.src, current_token(p).pos, "expected module declaration", no_print_line = current_token(p).kind == .EOF)
    }

    advance_token(p)
    // module name;
    //        ^~~^
    if current_token(p).kind != .identifier {
        if current_token(p).kind == .identifier_discard {
            error(p.file.path, p.lex.src, current_token(p).pos, "module name cannot be blank identifer \"_\"" )
        } else {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected module name (identifer)" )
        }
    }

    p.file.module.name = get_substring(p.file.src, current_token(p).pos)
    
    advance_token(p)

    if !consume_semicolon {
        
        module_declaration.end = current_token(p)
        add_global_stmt(p.file, module_declaration)
        
        return
    }

    // module name;
    //            ^
    if current_token(p).kind != .semicolon {
        error(p.file.path, p.lex.src, current_token(p).pos, "expected semicolon after module declaration, got %s", current_token(p).kind)
    }

    module_declaration.end = current_token(p)
    //

    advance_token(p)

    return module_declaration
}

parse_external_block :: proc(p: ^parser) {
    
}

parse_stmt :: proc(p: ^parser) {

    //TODO("fucking everything")

    #partial switch current_token(p).kind {
    case .keyword_module:
        if len(p.node_stack) == 0 {
            error(p.file.path, p.lex.src, current_token(p).pos, "module already declared", no_print_line = current_token(p).kind == .EOF)
        }
        
    case .semicolon:
        // empty statment
        advance_token(p)
    case .open_bracket:
        parse_block_stmt(p)
    case:
    }
}

parse_block_stmt :: proc(p: ^parser) {

}

parse_unary_expr :: proc(p: ^parser) -> (node: AST) {
    #partial switch current_token(p).kind {
    case .and:
        TODO("AMPERSAND UNARY EXPR")
    case .identifier:
        node = new(ident_expr)
        node.(^ident_expr).ident = get_substring(p.file.src, current_token(p).pos)
        return

    case:
        TODO("OOPSIE POOPSIE SANDWICH MADE AN OOPSIE (contact me)")
    }

    return
}

parse_unary_expr :: proc(p: ^parser) -> (node: AST) {
    #partial switch current_token(p).kind {
    case .and:
        TODO("AMPERSAND UNARY EXPR")
    case .identifier:
        node = new(ident_expr)
        node.(^ident_expr).ident = get_substring(p.file.src, current_token(p).pos)
        return

    case:
        TODO("OOPSIE POOPSIE SANDWICH MADE AN OOPSIE (contact me)")
    }

    return
}

parse_unary_expr :: proc(p: ^parser) -> (node: AST) {
    #partial switch current_token(p).kind {
    case .and:
        TODO("AMPERSAND UNARY EXPR")
    case .identifier:
        node = new(ident_expr)
        node.(^ident_expr).ident = get_substring(p.file.src, current_token(p).pos)
        return

    case:
        TODO("OOPSIE POOPSIE SANDWICH MADE AN OOPSIE (contact me)")
    }

    return
}

// merges a start and end position into a single position encompassing both.
merge_pos :: #force_inline proc(start, end : position) -> position {
    return {start.start, end.offset, start.line, start.col}
}