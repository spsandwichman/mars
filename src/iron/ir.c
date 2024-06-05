#include "iron.h"

#include "iron/passes/passes.h"

FeModule* air_new_module(string name) {
    FeModule* mod = mars_alloc(sizeof(*mod));

    da_init(&mod->symtab, 32);

    da_init(&mod->assembly, 32);

    air_typegraph_init(&mod->typegraph);

    da_init(&mod->pass_queue, 4);
    atlas_sched_pass(mod, &air_pass_canon);

    return mod;
}

// if (sym == NULL), create new symbol with no name
FeFunction* air_new_function(FeModule* mod, FeSymbol* sym, u8 visibility) {
    FeFunction* fn = mars_alloc(sizeof(FeFunction));

    fn->sym = sym ? sym : air_new_symbol(mod, NULL_STR, visibility, true, fn);
    fn->alloca = arena_make(FE_FN_ALLOCA_BLOCK_SIZE);
    da_init(&fn->blocks, 1);
    da_init(&fn->stack, 1);
    fn->entry_idx = 0;
    fn->params = NULL;
    fn->returns = NULL;
    fn->mod = mod;

    mod->functions = mars_realloc(mod->functions, sizeof(*mod->functions) * (mod->functions_len+1));
    mod->functions[mod->functions_len++] = fn;
    return fn;
}

FeStackObject* air_new_stackobject(FeFunction* f, FeType* t) {
    FeStackObject* obj = arena_alloc(&f->alloca, sizeof(*obj), alignof(*obj));
    obj->t = t;
    da_append(&f->stack, obj);
    return obj;
}

// takes multiple FE_Type*
void air_set_func_params(FeFunction* f, u16 count, ...) {
    f->params_len = count;

    if (f->params) mars_free(f->params);

    f->params = mars_alloc(sizeof(*f->params) * count);
    if (!f->params) CRASH("mars_alloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        FeFunctionItem* item = mars_alloc(sizeof(FeFunctionItem));
        if (!item) CRASH("item mars_alloc failed");
        
        if (!no_set) {
            item->T = va_arg(args, FeType*);
            if (item->T == NULL) {
                no_set = true;
            }
        }
        
        f->params[i] = item;
    }
    va_end(args);
}

// takes multiple entity*
void air_set_func_returns(FeFunction* f, u16 count, ...) {
    f->returns_len = count;

    if (f->returns) mars_free(f->returns);

    f->returns = mars_alloc(sizeof(*f->returns) * count);
    if (!f->params) CRASH("mars_alloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        FeFunctionItem* item = mars_alloc(sizeof(FeFunctionItem));
        if (!item) CRASH("item mars_alloc failed");
        
        if (!no_set) {
            item->T = va_arg(args, FeType*);
            if (item->T == NULL) {
                no_set = true;
            }
        }
        
        f->returns[i] = item;
    }
    va_end(args);
}

// if (sym == NULL), create new symbol with default name
FeGlobal* air_new_global(FeModule* mod, FeSymbol* sym, bool global, bool read_only) {
    FeGlobal* gl = mars_alloc(sizeof(FeGlobal));

    gl->sym = sym ? sym : air_new_symbol(mod, strprintf("symbol%zu", sym), global, false, gl);
    gl->read_only = read_only;
    gl->data = NULL;
    gl->data_len = 0;

    mod->globals = mars_realloc(mod->globals, sizeof(*mod->globals) * (mod->globals_len+1));
    mod->globals[mod->globals_len++] = gl;
    return gl;
}

void air_set_global_data(FeGlobal* global, u8* data, u32 data_len, bool zeroed) {
    global->is_symbol_ref = false;
    global->data = data;
    global->data_len = data_len;
    global->zeroed = zeroed;
}

void air_set_global_symref(FeGlobal* global, FeSymbol* symref) {
    global->is_symbol_ref = true;
    global->symref = symref;
}

// WARNING: does NOT check if a symbol already exists
// in most cases, use air_find_or_create_symbol
FeSymbol* air_new_symbol(FeModule* mod, string name, u8 visibility, bool function, void* ref) {
    FeSymbol* sym = mars_alloc(sizeof(FeSymbol));
    sym->name = name;
    sym->ref = ref;
    sym->is_function = function;
    sym->visibility = visibility;

    da_append(&mod->symtab, sym);
    return sym;
}

// use this instead of air_new_symbol
FeSymbol* air_find_or_new_symbol(FeModule* mod, string name, u8 visibility, bool function, void* ref) {
    FeSymbol* sym = air_find_symbol(mod, name);
    return sym ? sym : air_new_symbol(mod, name, visibility, function, ref);
}

FeSymbol* air_find_symbol(FeModule* mod, string name) {
    for_urange(i, 0, mod->symtab.len) {
        if (string_eq(mod->symtab.at[i]->name, name)) {
            return mod->symtab.at[i];
        }
    }
    return NULL;
}

FeBasicBlock* air_new_basic_block(FeFunction* fn, string name) {
    FeBasicBlock* bb = mars_alloc(sizeof(FeBasicBlock));
    if (!bb) CRASH("mars_alloc failed");

    bb->name = name;
    da_init(bb, 4);

    da_append(&fn->blocks, bb);
    return bb;
}

u32 air_bb_index(FeFunction* fn, FeBasicBlock* bb) {
    for_urange(i, 0, fn->blocks.len) {
        if (fn->blocks.at[i] != bb) continue;

        return i;
    }

    return UINT32_MAX;
}

FeInst* air_add(FeBasicBlock* bb, FeInst* ir) {
    ir->bb = bb;
    da_append(bb, ir);
    return ir;
}

FeInst* air_make(FeFunction* f, u8 type) {
    if (type >= FE_INSTR_COUNT) type = FE_INVALID;
    FeInst* ir = arena_alloc(&f->alloca, air_sizes[type], 8);
    ir->tag = type;
    ir->T = air_new_type(f->mod->am, FE_VOID, 0);
    ir->number = 0;
    return ir;
}

const size_t air_sizes[] = {
    [FE_INVALID]    = 0,
    [FE_ELIMINATED] = 0,

    [FE_ADD] = sizeof(FeBinop),
    [FE_SUB] = sizeof(FeBinop),
    [FE_MUL] = sizeof(FeBinop),
    [FE_DIV] = sizeof(FeBinop),
    
    [FE_AND]   = sizeof(FeBinop),
    [FE_OR]    = sizeof(FeBinop),
    [FE_NOR]   = sizeof(FeBinop),
    [FE_XOR]   = sizeof(FeBinop),
    [FE_SHL]   = sizeof(FeBinop),
    [FE_LSR]   = sizeof(FeBinop),
    [FE_ASR]   = sizeof(FeBinop),

    [FE_STACKOFFSET] = sizeof(FeStackOffset),
    [FE_GETFIELDPTR] = sizeof(FeGetFieldPtr),
    [FE_GETINDEXPTR] = sizeof(FeGetIndexPtr),

    [FE_LOAD]     = sizeof(FeLoad),
    [FE_VOL_LOAD] = sizeof(FeLoad),

    [FE_STORE]     = sizeof(FeStore),
    [FE_VOL_STORE] = sizeof(FeStore),

    [FE_CONST]      = sizeof(FeConst),
    [FE_LOADSYMBOL] = sizeof(FeLoadSymbol),

    [FE_MOV] = sizeof(FeMov),
    [FE_PHI] = sizeof(FePhi),

    [FE_BRANCH] = sizeof(FeBranch),
    [FE_JUMP]   = sizeof(FeJump),

    [FE_PARAMVAL]  = sizeof(FeParamVal),
    [FE_RETURNVAL] = sizeof(FeReturnVal),

    [FE_RETURN] = sizeof(FeReturn),
};

FeInst* air_make_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs) {
    FeBinop* ir = (FeBinop*) air_make(f, type);
    
    ir->lhs = lhs;
    ir->rhs = rhs;
    return (FeInst*) ir;
}

FeInst* air_make_cast(FeFunction* f, FeInst* source, FeType* to) {
    FeCast* ir = (FeCast*) air_make(f, FE_CAST);
    ir->source = source;
    ir->to = to;
    return (FeInst*) ir;
}

FeInst* air_make_stackoffset(FeFunction* f, FeStackObject* obj) {
    FeStackOffset* ir = (FeStackOffset*) air_make(f, FE_STACKOFFSET);

    ir->object = obj;
    return (FeInst*) ir;
}

FeInst* air_make_getfieldptr(FeFunction* f, u32 index, FeInst* source) {
    FeGetFieldPtr* ir = (FeGetFieldPtr*) air_make(f, FE_GETFIELDPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* air_make_getindexptr(FeFunction* f, FeInst* index, FeInst* source) {
    FeGetIndexPtr* ir = (FeGetIndexPtr*) air_make(f, FE_GETINDEXPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* air_make_load(FeFunction* f, FeInst* location, bool is_vol) {
    FeLoad* ir = (FeLoad*) air_make(f, FE_LOAD);

    if (is_vol) ir->base.tag = FE_VOL_LOAD;
    ir->location = location;
    return (FeInst*) ir;
}

FeInst* air_make_store(FeFunction* f, FeInst* location, FeInst* value, bool is_vol) {
    FeStore* ir = (FeStore*) air_make(f, FE_STORE);
    
    if (is_vol) ir->base.tag = FE_VOL_STORE;
    ir->location = location;
    ir->value = value;
    return (FeInst*) ir;
}

FeInst* air_make_const(FeFunction* f) {
    FeConst* ir = (FeConst*) air_make(f, FE_CONST);
    return (FeInst*) ir;
}

FeInst* air_make_loadsymbol(FeFunction* f, FeSymbol* symbol) {
    FeLoadSymbol* ir = (FeLoadSymbol*) air_make(f, FE_LOADSYMBOL);
    ir->sym = symbol;
    return (FeInst*) ir;
}

FeInst* air_make_mov(FeFunction* f, FeInst* source) {
    FeMov* ir = (FeMov*) air_make(f, FE_MOV);
    ir->source = source;
    return (FeInst*) ir;
}

// use in the format (f, source_count, source_1, source_BB_1, source_2, source_BB_2, ...)
FeInst* air_make_phi(FeFunction* f, u32 count, ...) {
    FePhi* ir = (FePhi*) air_make(f, FE_PHI);
    ir->len = count;

    ir->sources    = mars_alloc(sizeof(*ir->sources) * count);
    ir->source_BBs = mars_alloc(sizeof(*ir->source_BBs) * count);

    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        ir->sources[i]    = va_arg(args, FeInst*);
        ir->source_BBs[i] = va_arg(args, FeBasicBlock*);
    }
    va_end(args);

    return (FeInst*) ir;
}

void air_add_phi_source(FePhi* phi, FeInst* source, FeBasicBlock* source_block) {
    // wrote this and then remembered mars_realloc exists. too late :3
    FeInst** new_sources    = mars_alloc(sizeof(*phi->sources) * (phi->len + 1));
    FeBasicBlock** new_source_BBs = mars_alloc(sizeof(*phi->source_BBs) * (phi->len + 1));

    if (!new_sources || !new_source_BBs) {
        CRASH("mars_alloc returned null");
    }

    memcpy(new_sources, phi->sources, sizeof(*phi->sources) * phi->len);
    memcpy(new_source_BBs, phi->source_BBs, sizeof(*phi->source_BBs) * phi->len);

    new_sources[phi->len]    = source;
    new_source_BBs[phi->len] = source_block;

    mars_free(phi->sources);
    mars_free(phi->source_BBs);

    phi->sources = new_sources;
    phi->source_BBs = new_source_BBs;
    phi->len++;
}

FeInst* air_make_jump(FeFunction* f, FeBasicBlock* dest) {
    FeJump* ir = (FeJump*) air_make(f, FE_JUMP);
    ir->dest = dest;
    return (FeInst*) ir;
}

FeInst* air_make_branch(FeFunction* f, u8 cond, FeInst* lhs, FeInst* rhs, FeBasicBlock* if_true, FeBasicBlock* if_false) {
    FeBranch* ir = (FeBranch*) air_make(f, FE_BRANCH);
    ir->cond = cond;
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (FeInst*) ir;
}

FeInst* air_make_paramval(FeFunction* f, u32 param) {
    FeParamVal* ir = (FeParamVal*) air_make(f, FE_PARAMVAL);
    ir->param_idx = param;
    return (FeInst*) ir;
}

FeInst* air_make_returnval(FeFunction* f, u32 param, FeInst* source) {
    FeReturnVal* ir = (FeReturnVal*) air_make(f, FE_RETURNVAL);
    ir->return_idx = param;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* air_make_return(FeFunction* f) {
    return air_make(f, FE_RETURN);
}

void air_move_element(FeBasicBlock* bb, u64 to, u64 from) {
    if (to == from) return;

    FeInst* from_elem = bb->at[from];
    if (to < from) {
        memmove(&bb->at[to+1], &bb->at[to], (from - to) * sizeof(FeInst*));
    } else {
        memmove(&bb->at[to], &bb->at[to+1], (to - from) * sizeof(FeInst*));
    }
    bb->at[to] = from_elem;
}


u32 air_renumber(FeFunction* f) {
    u32 count = 0;
    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            f->blocks.at[i]->at[j]->number = ++count;
        }
    }
    return count;
}

void air_print_function(FeFunction* f) {
    air_renumber(f);
    
    printf("fn \""str_fmt"\" ", str_arg(f->sym->name));

    printf("(");
    for_urange(i, 0, f->params_len) {
        // string typestr = type_to_string(f->params[i]->e->entity_type);
        // printf(str_fmt, str_arg(typestr));

        if (i + 1 != f->params_len) {
            printf(", ");
        }
    }

    printf(") -> (");
    for_urange(i, 0, f->returns_len) {
        // string typestr = type_to_string(f->returns[i]->e->entity_type);
        // printf(str_fmt, str_arg(typestr));
        
        if (i + 1 != f->returns_len) {
            printf(", ");
        }
    }

    printf(") {\n");
    for_urange(i, 0, f->blocks.len) {
        printf("    ");
        if (f->entry_idx == i) printf("entry ");
        air_print_bb(f->blocks.at[i]);
    }
    printf("}\n");
}

void air_print_bb(FeBasicBlock* bb) {
    printf(str_fmt":\n", str_arg(bb->name));

    for_urange(i, 0, bb->len) {
        printf("        ");
        air_print_ir(bb->at[i]);
        printf("\n");
    }
}

void air_print_ir(FeInst* ir) {
    
    if (!ir) {
        printf("[null]");
        return;
    }

    char* binopstr; //hacky but w/e

    string typestr = constr("TODO TYPE2STR");//type_to_string(ir->T);
    printf("#%-3zu %-4.*s = ", ir->number, str_arg(typestr));
    switch (ir->tag) {
    case FE_INVALID: 
        printf("invalid!");
        break;

    case FE_ELIMINATED:
        printf("---");
        return;

    if(0){case FE_ADD:   binopstr = "add";} //common path fallthroughs
    if(0){case FE_SUB:   binopstr = "sub";}
    if(0){case FE_MUL:   binopstr = "mul";}
    if(0){case FE_DIV:   binopstr = "div";}
    if(0){case FE_AND:   binopstr = "and";}
    if(0){case FE_OR:    binopstr = "or";} 
    if(0){case FE_NOR:   binopstr = "nor";}
    if(0){case FE_XOR:   binopstr = "xor";}
    if(0){case FE_SHL:   binopstr = "shl";}
    if(0){case FE_LSR:   binopstr = "lsr";}
    if(0){case FE_ASR:   binopstr = "asr";}
        FeBinop* binop = (FeBinop*) ir;
        printf("%s #%zu, #%zu", binopstr, binop->lhs->number, binop->rhs->number);
        break;
    
    case FE_PARAMVAL:
        FeParamVal* paramval = (FeParamVal*) ir;
        printf("paramval <%zu>", paramval->param_idx);
        break;

    case FE_RETURNVAL:
        FeReturnVal* returnval = (FeReturnVal*) ir;
        printf("returnval <%zu> #%zu", returnval->return_idx, returnval->source->number);
        break;

    case FE_RETURN:
        printf("return");
        break;

    case FE_STACKOFFSET:
        FeStackOffset* stackoffset = (FeStackOffset*) ir;
        // string typestr = type_to_string(stackalloc->alloctype);
        // CRASH("");
        string typestr = str("TODO");
        printf("stackoffset <"str_fmt">", str_arg(typestr));
        break;

    case FE_STORE:
        FeStore* store = (FeStore*) ir;
        printf("store #%zu, #%zu", store->location->number, store->value->number);
        break;
    
    case FE_LOAD:
        FeLoad* load = (FeLoad*) ir;
        printf("load #%zu", load->location->number);
        break;

    case FE_MOV:
        FeMov* mov = (FeMov*) ir;
        printf("mov #%zu", mov->source->number);
        break;

    case FE_CONST:
        FeConst* con = (FeConst*) ir;

        // string typestr_ = type_to_string(con->base.T);
        // printf("const <"str_fmt", ", str_arg(typestr_));

        switch (con->base.T->kind) {
        case FE_I8:  printf("%lld", (i64)con->i8);  break;
        case FE_I16: printf("%lld", (i64)con->i16); break;
        case FE_I32: printf("%lld", (i64)con->i32); break;
        case FE_I64: printf("%lld", (i64)con->i64); break;
        case FE_U8:  printf("%llu", (u64)con->u8);  break;
        case FE_U16: printf("%llu", (u64)con->u16); break;
        case FE_U32: printf("%llu", (u64)con->u32); break;
        case FE_U64: printf("%llu", (u64)con->u64); break;
        case FE_F16: printf("%f",   (f32)con->f16); break;
        case FE_F32: printf("%f",   con->f32); break;
        case FE_F64: printf("%lf",  con->f64); break;
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

void air_print_module(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        air_print_function(mod->functions[i]);
    }
}

static char* write_str(char* buf, char* src) {
    memcpy(buf, src, strlen(src));
    return buf + strlen(src);
}