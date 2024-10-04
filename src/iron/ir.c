#include "iron/iron.h"
#include "iron/passes/passes.h"
#include "iron/codegen/x64/x64.h"

#define FE_FATAL(m, msg) fe_push_report(m, (FeReport){ \
    .function_of_origin = __func__,\
    .message = (msg),\
    .severity = FE_MSG_SEVERITY_FATAL, \
})

// if (sym == NULL), create new symbol with no name
FeFunction* fe_new_function(FeModule* mod, FeSymbol* sym, u8 cconv) {
    FeFunction* fn = fe_malloc(sizeof(FeFunction));

    fn->sym = sym ? sym : fe_new_symbol(mod, NULL_STR, FE_BIND_EXPORT);
    if (!sym) fn->sym->name = strprintf("symbol_%016llx", fn->sym);
    fn->sym->is_function = true;
    fn->sym->function = fn;
    fn->cconv = cconv;

    fn->alloca = arena_make(FE_FN_ALLOCA_BLOCK_SIZE);
    da_init(&fn->blocks, 1);
    da_init(&fn->stack, 1);
    fn->params.at = NULL;
    fn->returns.at = NULL;
    fn->mod = mod;

    mod->functions = mars_realloc(mod->functions, sizeof(*mod->functions) * (mod->functions_len+1));
    mod->functions[mod->functions_len++] = fn;
    return fn;
}

FeStackObject* fe_new_stackobject(FeFunction* f, FeType t) {
    FeStackObject* obj = arena_alloc(&f->alloca, sizeof(*obj), alignof(*obj));
    obj->t = t;
    da_append(&f->stack, obj);
    return obj;
}

static char* cstrprintf(char* fmt, ...) {

    va_list varargs;
    va_start(varargs, fmt);

    static char buf[1000];
    memset(buf, 0, sizeof(buf));

    vsprintf(buf, fmt, varargs);
    char* out = malloc(strlen(buf));
    strcpy(out, buf);
    return out;
}

void fe_init_func_params(FeFunction* f, u16 count) {
    f->params.at = fe_malloc(count * sizeof(f->params.at[0]));
    f->params.cap = count;
    f->params.len = 0;
}
void fe_init_func_returns(FeFunction* f, u16 count) {
    f->returns.at = fe_malloc(count * sizeof(f->returns.at[0]));
    f->returns.cap = count;
    f->returns.len = 0;
}
FeFunctionItem* fe_add_func_param(FeFunction* f, FeType t) {
    if (f->params.at == NULL) {
        fe_init_func_params(f, 4);
    }
    if (f->params.len == f->params.cap) {
        f->params.at = mars_realloc(f->params.at, f->params.cap * 2);
        f->params.cap *= 2;
    }
    FeFunctionItem* p = fe_malloc(sizeof(*p));
    p->type = t;
    f->params.at[f->params.len++] = p;
    return p;
}
FeFunctionItem* fe_add_func_return(FeFunction* f, FeType t) {
    if (f->returns.at == NULL) {
        fe_init_func_params(f, 4);
    }
    if (f->returns.len == f->returns.cap) {
        f->returns.at = mars_realloc(f->returns.at, f->returns.cap * 2);
        f->returns.cap *= 2;
    }
    FeFunctionItem* r = fe_malloc(sizeof(*r));
    r->type = t;
    f->returns.at[f->returns.len++] = r;
    return r;
}

FeData* fe_new_data(FeModule* mod, FeSymbol* sym, bool read_only) {
    FeData* data = fe_malloc(sizeof(FeData));

    data->sym = sym;
    data->read_only = read_only;

    mod->datas = mars_realloc(mod->datas, sizeof(*mod->datas) * (mod->datas_len+1));
    mod->datas[mod->datas_len++] = data;
    return data;
}

void fe_set_data_bytes(FeData* data, u8* bytes, u32 len, bool zeroed) {
    data->kind = FE_DATA_BYTES;
    data->bytes.data = bytes;
    data->bytes.len = len;
    data->bytes.zeroed = zeroed;
}

void fe_set_data_symref(FeData* data, FeSymbol* symref) {
    data->kind = FE_DATA_SYMREF;
    data->symref = symref;
}

void fe_set_data_numeric(FeData* data, u64 content, u8 kind) {
    if (!(kind > _FE_DATA_NUMERIC_BEGIN && kind < _FE_DATA_NUMERIC_END)) {
        CRASH("data kind %d is not numeric", kind);
    }
    data->kind = kind;
}

// WARNING: does NOT check if a symbol already exists
FeSymbol* fe_new_symbol(FeModule* mod, string name, u8 binding) {
    FeSymbol* sym = fe_malloc(sizeof(FeSymbol));
    sym->name = name;
    sym->binding = binding;

    da_append(&mod->symtab, sym);
    return sym;
}

FeSymbol* fe_find_or_new_symbol(FeModule* mod, string name, u8 binding) {
    FeSymbol* sym = fe_find_symbol(mod, name);
    return sym ? sym : fe_new_symbol(mod, name, binding);
}

// returns NULL if the symbol cannot be found
FeSymbol* fe_find_symbol(FeModule* mod, string name) {
    for_urange(i, 0, mod->symtab.len) {
        if (string_eq(mod->symtab.at[i]->name, name)) {
            return mod->symtab.at[i];
        }
    }
    return NULL;
}

FeBasicBlock* fe_new_basic_block(FeFunction* fn, string name) {
    FeBasicBlock* bb = fe_malloc(sizeof(FeBasicBlock));

    bb->name = name;
    bb->function = fn;

    FeIrBookend* bk = (FeIrBookend*) fe_ir(fn, FE_IR_BOOKEND);
    bk->bb = bb;

    bb->start = (FeIr*) bk;
    bb->end = (FeIr*) bk;

    da_append(&fn->blocks, bb);
    return bb;
}

u32 fe_bb_index(FeFunction* fn, FeBasicBlock* bb) {
    for_urange(i, 0, fn->blocks.len) {
        if (fn->blocks.at[i] != bb) continue;
        return i;
    }
    return UINT32_MAX;
}

FeIr* fe_append_ir(FeBasicBlock* bb, FeIr* inst) {
    inst->prev = NULL;
    inst->next = NULL;

    // this is the basic block's first instruction.
    if (bb->start->kind == FE_IR_BOOKEND) {
        FeIr* bookend = bb->start;
        // set up the bookend
        bookend->next = inst;
        bookend->prev = inst;
        inst->next = bookend;
        inst->prev = bookend;

        // retarget bb off of the bookend
        bb->start = inst;
        bb->end = inst;
    } else {
        fe_insert_ir_after(inst, bb->end);
    }

    return inst;
}

// inserts new before ref
FeIr* fe_insert_ir_before(FeIr* new, FeIr* ref) {
    if (ref->prev->kind == FE_IR_BOOKEND) {
        FeIrBookend* bookend = (FeIrBookend*)ref->prev;
        bookend->bb->start = new;
    }
    new->next = ref;
    new->prev = ref->prev;
    ref->prev->next = new;
    ref->prev = new;
    
    return new;
}

// inserts new after ref
FeIr* fe_insert_ir_after(FeIr* new, FeIr* ref) {
    if (ref->next->kind == FE_IR_BOOKEND) {
        FeIrBookend* bookend = (FeIrBookend*)ref->next;
        bookend->bb->end = new;
    }
    new->prev = ref;
    new->next = ref->next;
    ref->next->prev = new;
    ref->next = new;

    return new;
}

FeIr* fe_ir(FeFunction* f, u16 type) {
    size_t extra = 0;
    if (type >= _FE_IR_ARCH_SPECIFIC_START) {
        if (f->mod->target.arch == 0) {
            FE_FATAL(f->mod, "cannot create arch-specific ir without target arch");
        }
        TODO("");
    }
    FeIr* ir = arena_alloc(&f->alloca, fe_inst_sizes[type] + extra, 8);
    ir->kind = type;
    ir->type = FE_TYPE_VOID;
    return ir;
}

const size_t fe_inst_sizes[] = {
    [FE_IR_INVALID] = sizeof(FeIr),
    [FE_IR_BOOKEND] = sizeof(FeIrBookend),

    [FE_IR_ADD]  = sizeof(FeIrBinop),
    [FE_IR_SUB]  = sizeof(FeIrBinop),
    [FE_IR_IMUL] = sizeof(FeIrBinop),
    [FE_IR_UMUL] = sizeof(FeIrBinop),
    [FE_IR_IDIV] = sizeof(FeIrBinop),
    [FE_IR_UDIV] = sizeof(FeIrBinop),

    [FE_IR_AND]  = sizeof(FeIrBinop),
    [FE_IR_OR]   = sizeof(FeIrBinop),
    [FE_IR_XOR]  = sizeof(FeIrBinop),
    [FE_IR_SHL]  = sizeof(FeIrBinop),
    [FE_IR_LSR]  = sizeof(FeIrBinop),
    [FE_IR_ASR]  = sizeof(FeIrBinop),
    
    [FE_IR_ULT]  = sizeof(FeIrBinop),
    [FE_IR_UGT]  = sizeof(FeIrBinop),
    [FE_IR_ULE]  = sizeof(FeIrBinop),
    [FE_IR_UGE]  = sizeof(FeIrBinop),
    [FE_IR_ILT]  = sizeof(FeIrBinop),
    [FE_IR_IGT]  = sizeof(FeIrBinop),
    [FE_IR_ILE]  = sizeof(FeIrBinop),
    [FE_IR_IGE]  = sizeof(FeIrBinop),
    [FE_IR_EQ]   = sizeof(FeIrBinop),
    [FE_IR_NE]   = sizeof(FeIrBinop),

    [FE_IR_NOT]     = sizeof(FeIrUnop),
    [FE_IR_NEG]     = sizeof(FeIrUnop),
    [FE_IR_BITCAST] = sizeof(FeIrUnop),
    [FE_IR_TRUNC]   = sizeof(FeIrUnop),
    [FE_IR_SIGNEXT] = sizeof(FeIrUnop),
    [FE_IR_ZEROEXT] = sizeof(FeIrUnop),

    [FE_IR_STACK_ADDR] = sizeof(FeIrStackAddr),
    [FE_IR_FIELDPTR]  = sizeof(FeIrFieldPtr),
    [FE_IR_INDEXPTR]  = sizeof(FeIrIndexPtr),

    [FE_IR_LOAD]        = sizeof(FeIrLoad),
    [FE_IR_VOL_LOAD]    = sizeof(FeIrLoad),
    [FE_IR_STACK_LOAD] = sizeof(FeIrStackLoad),

    [FE_IR_STORE]       = sizeof(FeIrStore),
    [FE_IR_VOL_STORE]   = sizeof(FeIrStore),
    [FE_IR_STACK_STORE] = sizeof(FeIrStackStore),

    [FE_IR_CONST]  = sizeof(FeIrConst),
    [FE_IR_LOAD_SYMBOL] = sizeof(FeIrLoadSymbol),

    [FE_IR_MOV] = sizeof(FeIrMov),
    [FE_IR_PHI] = sizeof(FeIrPhi),

    [FE_IR_BRANCH] = sizeof(FeIrBranch),
    [FE_IR_JUMP]   = sizeof(FeIrJump),

    [FE_IR_PARAMVAL]  = sizeof(FeIrParamVal),
    [FE_IR_RETURNVAL] = sizeof(FeIrReturnVal),

    [FE_IR_RETURN] = sizeof(FeIrReturn),
};

FeIr* fe_ir_binop(FeFunction* f, u16 type, FeIr* lhs, FeIr* rhs) {
    FeIrBinop* ir = (FeIrBinop*) fe_ir(f, type);

    if (lhs->type == rhs->type) {
        ir->base.type = lhs->type;
    } else {
        FE_FATAL(f->mod, "lhs type != rhs type");
    }
    if (_FE_IR_CMP_START < type && type < _FE_IR_CMP_END) {
        ir->base.type = FE_TYPE_BOOL;
    }

    switch (type) {
    case FE_IR_ADD:
    case FE_IR_UMUL:
    case FE_IR_IMUL:
        if (lhs->kind == FE_IR_CONST && rhs->kind != FE_IR_CONST) {
            ir->lhs = rhs;
            ir->rhs = lhs;
            break;
        }
    default:
        ir->lhs = lhs;
        ir->rhs = rhs;
        break;
    }

    return (FeIr*) ir;
}

FeIr* fe_ir_unop(FeFunction* f, u16 type, FeIr* source) {
    FeIrUnop* ir = (FeIrUnop*) fe_ir(f, type);
    ir->source = source;
    return (FeIr*) ir;
}

FeIr* fe_ir_stackaddr(FeFunction* f, FeStackObject* obj) {
    FeIrStackAddr* ir = (FeIrStackAddr*) fe_ir(f, FE_IR_STACK_ADDR);

    ir->object = obj;
    return (FeIr*) ir;
}

FeIr* fe_ir_getfieldptr(FeFunction* f, u32 index, FeIr* source) {
    FeIrFieldPtr* ir = (FeIrFieldPtr*) fe_ir(f, FE_IR_FIELDPTR);
    ir->index = index;
    ir->source = source;
    return (FeIr*) ir;
}

FeIr* fe_ir_getindexptr(FeFunction* f, FeIr* index, FeIr* source) {
    FeIrIndexPtr* ir = (FeIrIndexPtr*) fe_ir(f, FE_IR_INDEXPTR);
    ir->index = index;
    ir->source = source;
    return (FeIr*) ir;
}

FeIr* fe_ir_load(FeFunction* f, FeIr* ptr, FeType as, bool is_vol) {
    FeIrLoad* ir = (FeIrLoad*) fe_ir(f, FE_IR_LOAD);
    if (!fe_type_is_scalar(as)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    if (is_vol) ir->base.kind = FE_IR_VOL_LOAD;
    ir->location = ptr;
    ir->base.type = as;
    return (FeIr*) ir;
}

FeIr* fe_ir_store(FeFunction* f, FeIr* ptr, FeIr* value, bool is_vol) {
    FeIrStore* ir = (FeIrStore*) fe_ir(f, FE_IR_STORE);
    if (!fe_type_is_scalar(value->type)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    if (is_vol) ir->base.kind = FE_IR_VOL_STORE;
    ir->location = ptr;
    ir->value = value;
    return (FeIr*) ir;
}

FeIr* fe_ir_stack_load(FeFunction* f, FeStackObject* location) {
    FeIrStackLoad* ir = (FeIrStackLoad*) fe_ir(f, FE_IR_STACK_LOAD);
    if (!fe_type_is_scalar(location->t)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    ir->base.type = location->t;
    ir->location = location;
    return (FeIr*) ir;
}

FeIr* fe_ir_stack_store(FeFunction* f, FeStackObject* location, FeIr* value) {
    FeIrStackStore* ir = (FeIrStackStore*) fe_ir(f, FE_IR_STACK_STORE);
    if (!fe_type_is_scalar(location->t)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    if (location->t != value->type) {
        FE_FATAL(f->mod, "value type != location type");
    }
    ir->location = location;
    ir->value = value;
    return (FeIr*) ir;
}

FeIr* fe_ir_const(FeFunction* f, FeType type) {
    FeIrConst* ir = (FeIrConst*) fe_ir(f, FE_IR_CONST);
    ir->base.type = type;
    return (FeIr*) ir;
}

FeIr* fe_ir_load_symbol(FeFunction* f, FeType type, FeSymbol* symbol) {
    FeIrLoadSymbol* ir = (FeIrLoadSymbol*) fe_ir(f, FE_IR_LOAD_SYMBOL);
    ir->sym = symbol;
    if (!fe_type_is_scalar(type)) {
        FE_FATAL(f->mod, "cannot load symbol into non-scalar type");
    }
    return (FeIr*) ir;
}

FeIr* fe_ir_mov(FeFunction* f, FeIr* source) {
    FeIrMov* ir = (FeIrMov*) fe_ir(f, FE_IR_MOV);
    ir->source = source;
    ir->base.type = source->type;
    return (FeIr*) ir;
}

FeIr* fe_ir_phi(FeFunction* f, u32 count, FeType type) {
    FeIrPhi* ir = (FeIrPhi*) fe_ir(f, FE_IR_PHI);
    ir->base.type = type;
    ir->len = 0;
    ir->cap = count;
    ir->sources    = fe_malloc(sizeof(*ir->sources)    * count);
    ir->source_BBs = fe_malloc(sizeof(*ir->source_BBs) * count);
    return (FeIr*) ir;
}

void fe_add_phi_source(FeFunction* f, FeIrPhi* phi, FeIr* source, FeBasicBlock* source_block) {
    if (source->type != phi->base.type) {
        FE_FATAL(f->mod, "phi source must match phi type");
    }

    if(phi->cap == phi->len) {
        phi->cap *= 2;
        phi->sources    = fe_realloc(phi->sources,    sizeof(*phi->sources)    * phi->cap);
        phi->source_BBs = fe_realloc(phi->source_BBs, sizeof(*phi->source_BBs) * phi->cap);
    }
    phi->sources   [phi->len] = source;
    phi->source_BBs[phi->len] = source_block;
    phi->len++;
}

FeIr* fe_ir_jump(FeFunction* f, FeBasicBlock* dest) {
    FeIrJump* ir = (FeIrJump*) fe_ir(f, FE_IR_JUMP);
    ir->dest = dest;
    return (FeIr*) ir;
}

FeIr* fe_ir_branch(FeFunction* f, FeIr* cond, FeBasicBlock* if_true, FeBasicBlock* if_false) {
    FeIrBranch* ir = (FeIrBranch*) fe_ir(f, FE_IR_BRANCH);
    ir->cond = cond;
    if (cond->type != FE_TYPE_BOOL) {
        FE_FATAL(f->mod, "branch condition must be boolean");
    }
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (FeIr*) ir;
}

FeIr* fe_ir_paramval(FeFunction* f, u32 param) {
    FeIrParamVal* ir = (FeIrParamVal*) fe_ir(f, FE_IR_PARAMVAL);
    ir->index = param;
    if (param >= f->params.len) {
        FE_FATAL(f->mod, cstrprintf("paramval index %d is out of range [0, %d)", param, f->params.len));
        // CRASH("paramval index %d is out of range", param);
    }
    ir->base.type = f->params.at[param]->type;
    return (FeIr*) ir;
}

FeIr* fe_ir_returnval(FeFunction* f, u32 ret, FeIr* source) {
    FeIrReturnVal* ir = (FeIrReturnVal*) fe_ir(f, FE_IR_RETURNVAL);
    ir->index = ret;
    if (ret >= f->returns.len) {
        FE_FATAL(f->mod, cstrprintf("returnval index %d is out of range [0, %d)", ret, f->returns.len));
    }
    ir->source = source;
    return (FeIr*) ir;
}

FeIr* fe_ir_return(FeFunction* f) {
    return fe_ir(f, FE_IR_RETURN);
}

// hasta la vista baby
bool fe_is_ir_terminator(FeIr* inst) {
    switch (inst->kind) {
    case FE_IR_RETURN:
    case FE_IR_BRANCH:
    case FE_IR_JUMP:
        return true;    
    default:
        return false;
    }
}

// remove inst from its basic block
FeIr* fe_remove_ir(FeIr* inst) {
    inst->prev->next = inst->next;
    inst->next->prev = inst->prev;
    return inst;
}

// move inst to before ref
FeIr* fe_move_ir_before(FeIr* inst, FeIr* ref) {
    if (inst == ref || ref->prev == inst) return inst;
    fe_remove_ir(inst);
    fe_insert_ir_before(inst, ref);
    return inst;
}

// move inst to before ref
FeIr* fe_move_ir_after(FeIr* inst, FeIr* ref) {
    if (inst == ref || ref->next == inst) return inst;
    fe_remove_ir(inst);
    fe_insert_ir_after(inst, ref);
    return inst;
}

// rewrite all uses of `from` to be uses of `to`

#define rewrite_if_eq(inst, source, dest) if (inst == source) inst = dest

void fe_rewrite_uses_in_inst(FeIr* inst, FeIr* source, FeIr* dest) {
    switch (inst->kind) {
    case FE_IR_ADD:
    case FE_IR_SUB:
    case FE_IR_IMUL:
    case FE_IR_UMUL:
    case FE_IR_IDIV:
    case FE_IR_UDIV:
    case FE_IR_AND:
    case FE_IR_OR:
    case FE_IR_XOR:
    case FE_IR_SHL:
    case FE_IR_LSR:
    case FE_IR_ASR:
    case FE_IR_ULT:
    case FE_IR_UGT:
    case FE_IR_ULE:
    case FE_IR_UGE:
    case FE_IR_ILT:
    case FE_IR_IGT:
    case FE_IR_ILE:
    case FE_IR_IGE:
    case FE_IR_EQ:
    case FE_IR_NE:
        FeIrBinop* binop = (FeIrBinop*) inst;
        rewrite_if_eq(binop->lhs, source, dest);
        rewrite_if_eq(binop->rhs, source, dest);
        break;
    case FE_IR_RETURNVAL:
        FeIrReturnVal* retval = (FeIrReturnVal*) inst;
        rewrite_if_eq(retval->source, source, dest);
        break;
    
    case FE_IR_STACK_STORE:
        FeIrStackStore* stack_store = (FeIrStackStore*) inst;
        rewrite_if_eq(stack_store->value, source, dest);
        break;
    case FE_IR_BRANCH:
        FeIrBranch* branch = (FeIrBranch*) inst;
        rewrite_if_eq(branch->cond, source, dest);
        break;

    case FE_IR_PHI:
        FeIrPhi* phi = (FeIrPhi*) inst;
        for_range(i, 0, phi->len) {
            rewrite_if_eq(phi->sources[i], source, dest);
        }
        break;
    case FE_IR_PARAMVAL:
    case FE_IR_STACK_ADDR:
    case FE_IR_STACK_LOAD:
    case FE_IR_CONST:
    case FE_IR_JUMP:
    case FE_IR_RETURN:
        break; // no inputs
    default:
        printf("---- %d\n", inst->kind);
        CRASH("unhandled in rewrite_uses_in_inst");
        break;
    }
}

void fe_rewrite_ir_uses(FeFunction* f, FeIr* source, FeIr* dest) {
    for_urange(i, 0, f->blocks.len) {
        FeBasicBlock* bb = f->blocks.at[i];
        for_fe_ir(inst, *bb) {
            fe_rewrite_uses_in_inst(inst, source, dest);
        }
    }
}

static FeIr* get_usage(FeBasicBlock* bb, FeIr* source, FeIr* start) {
    for_fe_ir_from(inst, start, *bb) {
        FeIr** ir = (FeIr**)inst;
        for (u64 j = sizeof(FeIr)/sizeof(FeIr*); j <= fe_inst_sizes[inst->kind]/sizeof(FeIr*); j++) {
            if (ir[j] == source) return inst;
        }
    }
    return NULL;
}

void fe_add_ir_uses_to_worklist(FeFunction* f, FeIr* source, da(FeIrPTR)* worklist) {
    for_urange(i, 0, f->blocks.len) {
        FeBasicBlock* bb = f->blocks.at[i];
        FeIr* next_usage = get_usage(bb, source, 0);
        while (next_usage != NULL) {
            da_append(worklist, next_usage);
            next_usage = get_usage(bb, source, next_usage->next);
        }
    }
}