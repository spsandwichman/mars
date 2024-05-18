#include "ir.h"

IR_Module* ir_new_module(string name) {
    IR_Module* mod = malloc(sizeof(IR_Module));

    mod->functions = NULL;
    mod->globals   = NULL;
    mod->functions_len = 0;
    mod->globals_len   = 0;

    mod->name = name;

    da_init(&mod->symtab, 4);
    return mod;
}

// if (sym == NULL), create new symbol with no name
IR_Function* ir_new_function(IR_Module* mod, IR_Symbol* sym, bool global) {
    IR_Function* fn = malloc(sizeof(IR_Function));

    fn->sym = sym ? sym : ir_new_symbol(mod, NULL_STR, global, true, fn);
    fn->alloca = arena_make(IR_FN_ALLOCA_BLOCK_SIZE);
    da_init(&fn->blocks, 1);
    fn->entry_idx = 0;
    // fn->exit_idx = 0;

    mod->functions = realloc(mod->functions, mod->functions_len+1);
    mod->functions[mod->functions_len++] = fn;
    return fn;
}

// takes multiple entity*
void ir_set_func_params(IR_Function* f, u16 count, ...) {
    f->params_len = count;

    if (f->params) free(f->params);

    f->params = malloc(sizeof(*f->params) * count);
    if (!f->params) CRASH("malloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        IR_FuncItem* item = malloc(sizeof(IR_FuncItem));
        if (!item) CRASH("item malloc failed");
        
        if (!no_set) {
            item->e = va_arg(args, Entity*);
            if (item->e == NULL) {
                no_set = true;
            }
        }
        
        f->params[i] = item;
    }
    va_end(args);
}

// takes multiple entity*
void ir_set_func_returns(IR_Function* f, u16 count, ...) {
    f->returns_len = count;

    if (f->returns) free(f->returns);

    f->returns = malloc(sizeof(*f->returns) * count);
    if (!f->params) CRASH("malloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        IR_FuncItem* item = malloc(sizeof(IR_FuncItem));
        if (!item) CRASH("item malloc failed");
        
        if (!no_set) {
            item->e = va_arg(args, Entity*);
            if (item->e == NULL) {
                no_set = true;
            }
        }
        
        f->returns[i] = item;
    }
    va_end(args);
}

// if (sym == NULL), create new symbol with default name
IR_Global* ir_new_global(IR_Module* mod, IR_Symbol* sym, bool global, bool read_only) {
    IR_Global* gl = malloc(sizeof(IR_Global));

    gl->sym = sym ? sym : ir_new_symbol(mod, strprintf("symbol%zu", sym), global, false, gl);
    gl->read_only = read_only;
    gl->data = NULL;
    gl->data_len = 0;

    mod->globals = realloc(mod->globals, mod->globals_len+1);
    mod->globals[mod->globals_len++] = gl;
    return gl;
}

void ir_set_global_data(IR_Global* global, u8* data, u32 data_len, bool zeroed) {
    global->is_symbol_ref = false;
    global->data = data;
    global->data_len = data_len;
    global->zeroed = zeroed;
}

void ir_set_global_symref(IR_Global* global, IR_Symbol* symref) {
    global->is_symbol_ref = true;
    global->symref = symref;
}

// WARNING: does NOT check if a symbol already exists
// in most cases, use ir_find_or_create_symbol
IR_Symbol* ir_new_symbol(IR_Module* mod, string name, u8 visibility, bool function, void* ref) {
    IR_Symbol* sym = malloc(sizeof(IR_Symbol));
    sym->name = name;
    sym->ref = ref;
    sym->is_function = function;
    sym->visibility = visibility;

    da_append(&mod->symtab, sym);
    return sym;
}

// use this instead of ir_new_symbol
IR_Symbol* ir_find_or_new_symbol(IR_Module* mod, string name, u8 visibility, bool function, void* ref) {
    IR_Symbol* sym = ir_find_symbol(mod, name);
    return sym ? sym : ir_new_symbol(mod, name, visibility, function, ref);
}

IR_Symbol* ir_find_symbol(IR_Module* mod, string name) {
    for_urange(i, 0, mod->symtab.len) {
        if (string_eq(mod->symtab.at[i]->name, name)) {
            return mod->symtab.at[i];
        }
    }
    return NULL;
}

IR_BasicBlock* ir_new_basic_block(IR_Function* fn, string name) {
    IR_BasicBlock* bb = malloc(sizeof(IR_BasicBlock));
    if (!bb) CRASH("malloc failed");

    bb->name = name;
    da_init(bb, 1);

    da_append(&fn->blocks, bb);
    return bb;
}

u32 ir_bb_index(IR_Function* fn, IR_BasicBlock* bb) {
    for_urange(i, 0, fn->blocks.len) {
        if (fn->blocks.at[i] != bb) continue;

        return i;
    }

    return UINT32_MAX;
}

IR* ir_add(IR_BasicBlock* bb, IR* ir) {
    ir->bb = bb;
    da_append(bb, ir);
    return ir;
}

IR* ir_make(IR_Function* f, u8 type) {
    if (type > IR_INSTR_COUNT) type = IR_INVALID;
    IR* ir = arena_alloc(&f->alloca, ir_sizes[type], 8);
    ir->tag = type;
    ir->T = NULL;
    ir->number = 0;
    return ir;
}

const size_t ir_sizes[] = {
    [IR_INVALID]    = 0,
    [IR_ELIMINATED] = 0,

    [IR_ADD] = sizeof(IR_BinOp),
    [IR_SUB] = sizeof(IR_BinOp),
    [IR_MUL] = sizeof(IR_BinOp),
    [IR_DIV] = sizeof(IR_BinOp),
    
    [IR_AND]   = sizeof(IR_BinOp),
    [IR_OR]    = sizeof(IR_BinOp),
    [IR_NOR]   = sizeof(IR_BinOp),
    [IR_XOR]   = sizeof(IR_BinOp),
    [IR_SHL]   = sizeof(IR_BinOp),
    [IR_LSR]   = sizeof(IR_BinOp),
    [IR_ASR]   = sizeof(IR_BinOp),
    [IR_TRUNC] = sizeof(IR_BinOp),
    [IR_SEXT]  = sizeof(IR_BinOp),
    [IR_ZEXT]  = sizeof(IR_BinOp),

    [IR_STACKALLOC]  = sizeof(IR_StackAlloc),
    [IR_GETFIELDPTR] = sizeof(IR_GetFieldPtr),
    [IR_GETINDEXPTR] = sizeof(IR_GetIndexPtr),

    [IR_LOAD]     = sizeof(IR_Load),
    [IR_VOL_LOAD] = sizeof(IR_Load),

    [IR_STORE]     = sizeof(IR_Store),
    [IR_VOL_STORE] = sizeof(IR_Store),

    [IR_CONST]      = sizeof(IR_Const),
    [IR_LOADSYMBOL] = sizeof(IR_LoadSymbol),

    [IR_MOV] = sizeof(IR_Mov),
    [IR_PHI] = sizeof(IR_Phi),

    [IR_BRANCH] = sizeof(IR_Branch),
    [IR_JUMP]   = sizeof(IR_Jump),

    [IR_PARAMVAL]  = sizeof(IR_ParamVal),
    [IR_RETURNVAL] = sizeof(IR_ReturnVal),

    [IR_RETURN] = sizeof(IR_Return),
};

IR* ir_make_binop(IR_Function* f, u8 type, IR* lhs, IR* rhs) {
    IR_BinOp* ir = (IR_BinOp*) ir_make(f, type);
    
    ir->lhs = lhs;
    ir->rhs = rhs;
    return (IR*) ir;
}

IR* ir_make_cast(IR_Function* f, IR* source, type* to) {
    IR_Cast* ir = (IR_Cast*) ir_make(f, IR_CAST);
    ir->source = source;
    ir->to = to;
    return (IR*) ir;
}

IR* ir_make_stackalloc(IR_Function* f, type* T) {
    IR_StackAlloc* ir = (IR_StackAlloc*) ir_make(f, IR_STACKALLOC);

    ir->alloctype = T;
    return (IR*) ir;
}

IR* ir_make_getfieldptr(IR_Function* f, u32 index, IR* source) {
    IR_GetFieldPtr* ir = (IR_GetFieldPtr*) ir_make(f, IR_GETFIELDPTR);
    ir->index = index;
    ir->source = source;
    return (IR*) ir;
}

IR* ir_make_getindexptr(IR_Function* f, IR* index, IR* source) {
    IR_GetIndexPtr* ir = (IR_GetIndexPtr*) ir_make(f, IR_GETINDEXPTR);
    ir->index = index;
    ir->source = source;
    return (IR*) ir;
}

IR* ir_make_load(IR_Function* f, IR* location, bool is_vol) {
    IR_Load* ir = (IR_Load*) ir_make(f, IR_LOAD);

    if (is_vol) ir->base.tag = IR_VOL_LOAD;
    ir->location = location;
    return (IR*) ir;
}

IR* ir_make_store(IR_Function* f, IR* location, IR* value, bool is_vol) {
    IR_Store* ir = (IR_Store*) ir_make(f, IR_STORE);
    
    if (is_vol) ir->base.tag = IR_VOL_STORE;
    ir->location = location;
    ir->value = value;
    return (IR*) ir;
}

IR* ir_make_const(IR_Function* f) {
    IR_Const* ir = (IR_Const*) ir_make(f, IR_CONST);
    return (IR*) ir;
}

IR* ir_make_loadsymbol(IR_Function* f, IR_Symbol* symbol) {
    IR_LoadSymbol* ir = (IR_LoadSymbol*) ir_make(f, IR_LOADSYMBOL);
    ir->sym = symbol;
    return (IR*) ir;
}

IR* ir_make_mov(IR_Function* f, IR* source) {
    IR_Mov* ir = (IR_Mov*) ir_make(f, IR_MOV);
    ir->source = source;
    return (IR*) ir;
}

// use in the format (f, source_count, source_1, source_BB_1, source_2, source_BB_2, ...)
IR* ir_make_phi(IR_Function* f, u32 count, ...) {
    IR_Phi* ir = (IR_Phi*) ir_make(f, IR_PHI);
    ir->len = count;

    ir->sources    = malloc(sizeof(*ir->sources) * count);
    ir->source_BBs = malloc(sizeof(*ir->source_BBs) * count);

    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        ir->sources[i]    = va_arg(args, IR*);
        ir->source_BBs[i] = va_arg(args, IR_BasicBlock*);
    }
    va_end(args);

    return (IR*) ir;
}

void ir_add_phi_source(IR_Phi* phi, IR* source, IR_BasicBlock* source_block) {
    // wrote this and then remembered realloc exists. too late :3
    IR** new_sources    = malloc(sizeof(*phi->sources) * (phi->len + 1));
    IR_BasicBlock** new_source_BBs = malloc(sizeof(*phi->source_BBs) * (phi->len + 1));

    if (!new_sources || !new_source_BBs) {
        CRASH("malloc returned null");
    }

    memcpy(new_sources, phi->sources, sizeof(*phi->sources) * phi->len);
    memcpy(new_source_BBs, phi->source_BBs, sizeof(*phi->source_BBs) * phi->len);

    new_sources[phi->len]    = source;
    new_source_BBs[phi->len] = source_block;

    free(phi->sources);
    free(phi->source_BBs);

    phi->sources = new_sources;
    phi->source_BBs = new_source_BBs;
    phi->len++;
}

IR* ir_make_jump(IR_Function* f, IR_BasicBlock* dest) {
    IR_Jump* ir = (IR_Jump*) ir_make(f, IR_JUMP);
    ir->dest = dest;
    return (IR*) ir;
}

IR* ir_make_branch(IR_Function* f, u8 cond, IR* lhs, IR* rhs, IR_BasicBlock* if_true, IR_BasicBlock* if_false) {
    IR_Branch* ir = (IR_Branch*) ir_make(f, IR_BRANCH);
    ir->cond = cond;
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (IR*) ir;
}

IR* ir_make_paramval(IR_Function* f, u32 param) {
    IR_ParamVal* ir = (IR_ParamVal*) ir_make(f, IR_PARAMVAL);
    ir->param_idx = param;
    return (IR*) ir;
}

IR* ir_make_returnval(IR_Function* f, u32 param, IR* source) {
    IR_ReturnVal* ir = (IR_ReturnVal*) ir_make(f, IR_RETURNVAL);
    ir->return_idx = param;
    ir->source = source;
    return (IR*) ir;
}

IR* ir_make_return(IR_Function* f) {
    return ir_make(f, IR_RETURN);
}

void ir_move_element(IR_BasicBlock* bb, u64 to, u64 from) {
    if (to == from) return;

    IR* from_elem = bb->at[from];
    if (to < from) {
        memmove(&bb->at[to+1], &bb->at[to], (from - to) * sizeof(IR*));
    } else {
        memmove(&bb->at[to], &bb->at[to+1], (to - from) * sizeof(IR*));
    }
    bb->at[to] = from_elem;
}


u32 ir_renumber(IR_Function* f) {
    u32 count = 0;
    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            f->blocks.at[i]->at[j]->number = ++count;
        }
    }
    return count;
}

void ir_print_function(IR_Function* f) {
    ir_renumber(f);
    
    printf("fn \""str_fmt"\" ", str_arg(f->sym->name));

    printf("(");
    for_urange(i, 0, f->params_len) {
        string typestr = type_to_string(f->params[i]->e->entity_type);
        printf(str_fmt, str_arg(typestr));

        if (i + 1 != f->params_len) {
            printf(", ");
        }
    }

    printf(") -> (");
    for_urange(i, 0, f->returns_len) {
        string typestr = type_to_string(f->returns[i]->e->entity_type);
        printf(str_fmt, str_arg(typestr));
        
        if (i + 1 != f->returns_len) {
            printf(", ");
        }
    }

    printf(") {\n");
    for_urange(i, 0, f->blocks.len) {
        printf("    ");
        if (f->entry_idx == i) printf("entry ");
        if (f->entry_idx == i) printf("exit ");
        ir_print_bb(f->blocks.at[i]);
    }
    printf("}\n");
}

void ir_print_bb(IR_BasicBlock* bb) {
    printf(str_fmt":\n", str_arg(bb->name));

    for_urange(i, 0, bb->len) {
        printf("        ");
        ir_print_ir(bb->at[i]);
        printf("\n");
    }
}

void ir_print_ir(IR* ir) {
    
    if (!ir) {
        printf("[null]");
        return;
    }

    char* binopstr; //hacky but w/e

    string typestr = type_to_string(ir->T);
    printf("#%-3zu %-4.*s = ", ir->number, str_arg(typestr));
    switch (ir->tag) {
    case IR_INVALID: 
        printf("invalid!");
        break;

    case IR_ELIMINATED:
        printf("---");
        return;

    if(0){case IR_ADD:   binopstr = "add";} //common path fallthroughs
    if(0){case IR_SUB:   binopstr = "sub";}
    if(0){case IR_MUL:   binopstr = "mul";}
    if(0){case IR_DIV:   binopstr = "div";}
    if(0){case IR_AND:   binopstr = "and";}
    if(0){case IR_OR:    binopstr = "or";} 
    if(0){case IR_NOR:   binopstr = "nor";}
    if(0){case IR_XOR:   binopstr = "xor";}
    if(0){case IR_SHL:   binopstr = "shl";}
    if(0){case IR_LSR:   binopstr = "lsr";}
    if(0){case IR_ASR:   binopstr = "asr";}
    if(0){case IR_TRUNC: binopstr = "trunc";}
    if(0){case IR_SEXT:  binopstr = "sext";}
    if(0){case IR_ZEXT:  binopstr = "zext";}
        IR_BinOp* binop = (IR_BinOp*) ir;
        printf("%s #%zu, #%zu", binopstr, binop->lhs->number, binop->rhs->number);
        break;
    
    case IR_PARAMVAL:
        IR_ParamVal* paramval = (IR_ParamVal*) ir;
        printf("paramval <%zu>", paramval->param_idx);
        break;

    case IR_RETURNVAL:
        IR_ReturnVal* returnval = (IR_ReturnVal*) ir;
        printf("returnval <%zu> #%zu", returnval->return_idx, returnval->source->number);
        break;

    case IR_RETURN:
        printf("return");
        break;

    case IR_STACKALLOC:
        IR_StackAlloc* stackalloc = (IR_StackAlloc*) ir;
        string typestr = type_to_string(stackalloc->alloctype);
        printf("stackalloc <"str_fmt">", str_arg(typestr));
        break;

    case IR_STORE:
        IR_Store* store = (IR_Store*) ir;
        printf("store #%zu, #%zu", store->location->number, store->value->number);
        break;
    
    case IR_LOAD:
        IR_Load* load = (IR_Load*) ir;
        printf("load #%zu", load->location->number);
        break;

    case IR_MOV:
        IR_Mov* mov = (IR_Mov*) ir;
        printf("mov #%zu", mov->source->number);
        break;

    case IR_CONST:
        IR_Const* con = (IR_Const*) ir;

        string typestr_ = type_to_string(con->base.T);
        printf("const <"str_fmt", ", str_arg(typestr_));

        switch (con->base.T->tag) {
        case TYPE_I8:  printf("%lld", (i64)con->i8);  break;
        case TYPE_I16: printf("%lld", (i64)con->i16); break;
        case TYPE_I32: printf("%lld", (i64)con->i32); break;
        case TYPE_I64: printf("%lld", (i64)con->i64); break;
        case TYPE_U8:  printf("%llu", (u64)con->u8);  break;
        case TYPE_U16: printf("%llu", (u64)con->u16); break;
        case TYPE_U32: printf("%llu", (u64)con->u32); break;
        case TYPE_U64: printf("%llu", (u64)con->u64); break;
        case TYPE_F16: printf("%f",   (f32)con->f16); break;
        case TYPE_F32: printf("%f",   con->f32); break;
        case TYPE_F64: printf("%lf",  con->f64); break;
        default:
            printf("???");
            break;
        }
        printf(">");
        break;

    default:
        printf("unimplemented %zu", (size_t)ir->tag);
        break;
    }
}

void ir_print_module(IR_Module* mod) {
    for_urange(i, 0, mod->functions_len) {
        ir_print_function(mod->functions[i]);
    }
}

static char* write_str(char* buf, char* src) {
    memcpy(buf, src, strlen(src));
    return buf + strlen(src);
}

static char* type2str_internal(type* t, char* buf) {
    if (t == NULL) {
        buf = write_str(buf, "[null]"); 
        return buf;
    }

    switch (t->tag) {
    case TYPE_NONE: return buf;
    case TYPE_BOOL: buf = write_str(buf, "bool"); return buf;
    case TYPE_U8:   buf = write_str(buf, "u8");  return buf;
    case TYPE_U16:  buf = write_str(buf, "u16"); return buf;
    case TYPE_U32:  buf = write_str(buf, "u32"); return buf;
    case TYPE_U64:  buf = write_str(buf, "u64"); return buf;
    case TYPE_I8:   buf = write_str(buf, "i8");  return buf;
    case TYPE_I16:  buf = write_str(buf, "i16"); return buf;
    case TYPE_I32:  buf = write_str(buf, "i32"); return buf;
    case TYPE_I64:  buf = write_str(buf, "i64"); return buf;
    case TYPE_F16:  buf = write_str(buf, "f16"); return buf;
    case TYPE_F32:  buf = write_str(buf, "f32"); return buf;
    case TYPE_F64:  buf = write_str(buf, "f64"); return buf;  
    case TYPE_POINTER:  buf = write_str(buf, "ptr"); return buf;  
    default: buf = write_str(buf, "unimpl"); return buf;
    }
}

string type_to_string(type* t) {
    // get a backing buffer
    char buf[500];
    memset(buf, 0, sizeof(buf));
    type2str_internal(t, buf);
    // allocate new buffer with concrete backing
    return string_clone(str(buf));
}