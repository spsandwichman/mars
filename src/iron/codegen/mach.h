#pragma once

#include "iron/iron.h"

typedef struct FeMachBuffer       FeMachBuffer;
typedef struct FeMachVReg         FeMachVReg;
typedef struct FeMachImmediate    FeMachImmediate;

typedef struct FeMach             FeMach;
typedef struct FeMachInst         FeMachInst;
typedef struct FeMachInstTemplate FeMachInstTemplate;

typedef u32 FeMachVregList;
typedef u32 FeMachImmediateList;
typedef u32 FeMachInstTemplateRef;

da_typedef(FeMachVReg);
da_typedef(u32);
da_typedef(FeMachImmediate);

typedef struct FeMachBuffer {
    u16 arch;
    u16 system;

    struct {
        FeMach** at;
        u64 len;
        u64 cap;
    } buf;

    da(u32) vreg_refs;
    da(FeMachVReg) vregs;
    da(FeMachImmediate) immediates;

    Arena buf_alloca;
} FeMachBuffer;

#define FE_MACH_VREG_UNASSIGNED (UINT16_MAX)

typedef struct FeMachVReg {
    u16 class;
    u16 real;
} FeMachVReg;

enum {
    // define a section
    FE_MACH_SECTION = 1,

    // artificially begin/end a vreg lifetime, as if an instruction had defined/used a value for it
    // useful for dictating saved registers across calls, making sure parameter and return registers
    // are correctly handled, anything to do with reservations and calling conventions really
    // FeMachLifetimePoint
    FE_MACH_LIFETIME_BEGIN,
    FE_MACH_LIFETIME_END,

    // usually at the start of a function, tells the register allocator to start working
    FE_MACH_REGALLOC_BEGIN,
    // usually at the end of a function, tells the register allocator to stop
    FE_MACH_REGALLOC_END,

    FE_MACH_LOCAL_LABEL,
    FE_MACH_SYMBOL_LABEL,

    FE_MACH_DIRECTIVE_ALIGN,

    FE_MACH_DATA_ZERO,
    FE_MACH_DATA_FILL,
    FE_MACH_DATA_D8,
    FE_MACH_DATA_D16,
    FE_MACH_DATA_D32,
    FE_MACH_DATA_D64,
    FE_MACH_DATA_BYTESTREAM,

    FE_MACH_INST,
};

typedef struct FeMach {
    u8 kind;
} FeMach;

typedef struct FeMachSection {
    FeMach base;

    
} FeMachSection;

typedef struct FeMachLifetimePoint {
    FeMach base;

    u32 vreg;
} FeMachLifetimePoint;

// machine instruction
typedef struct FeMachInst {
    FeMach base;

    FeMachInstTemplateRef template; // instruction template index
    FeMachVregList uses;
    FeMachVregList defs;
    FeMachImmediateList imms;
} FeMachInst;

enum {
    FE_MACH_IMM_CONST = 1,
    // FE_MACH_IMM_SYMBOL,
};

typedef struct FeMachImmediate {
    u8 kind;
    union {
        u64 d64;
        u32 d32;
        u16 d16;
        u8  d8;
    };
} FeMachImmediate;

// template for a machine instruction.
typedef struct FeMachInstTemplate {
    u16 inst;
    u8 uses_len;
    u8 defs_len;
    u8 imms_len;
    u8 size;
    bool side_effects;
} FeMachInstTemplate;