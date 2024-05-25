#pragma once
#define DEIMOS_IRGEN_H

#include "deimos.h"
#include "deimos/passes/passes.h"
#include "phobos/sema.h"

IR_Module* ir_generate(MarsModule* mod);
IR_Global* ir_generate_global_from_stmt_decl(IR_Module* mod, Ast ast);
IR* ir_generate_expr_literal(IR_Function* f, IR_BasicBlock* bb, Ast ast);
IR* ir_generate_expr_value(IR_Function* f, IR_BasicBlock* bb, Ast ast);
IR* ir_generate_expr_binop(IR_Function* f, IR_BasicBlock* bb, Ast ast);
IR* ir_generate_expr_ident_load(IR_Function* f, IR_BasicBlock* bb, Ast ast);
IR* ir_generate_expr_value(IR_Function* f, IR_BasicBlock* bb, Ast ast);
IR* ir_generate_expr_address(IR_Function* f, IR_BasicBlock* bb, Ast ast);
void ir_generate_stmt_assign(IR_Function* f, IR_BasicBlock* bb, Ast ast);
void ir_generate_stmt_return(IR_Function* f, IR_BasicBlock* bb, Ast ast);
IR_Function* ir_generate_function(IR_Module* mod, Ast ast);