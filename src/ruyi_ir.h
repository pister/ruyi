//
//  ruyi_ir.h
//  ruyi
//
//  Created by Songli Huang on 2019/11/7.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_ir_h
#define ruyi_ir_h

#include "ruyi_basics.h"
#include "ruyi_unicode.h"

#define RUYI_IR_INS_NAME_LENGTH 16


typedef enum {
    // basic types
    Ruyi_ir_type_Void = 0,  // v-0
    Ruyi_ir_type_Byte,      // b-8bit
    Ruyi_ir_type_Int16,     // s-16bit
    Ruyi_ir_type_Rune,      // r-32bit
    Ruyi_ir_type_Int32,     // i-32bit
    Ruyi_ir_type_Int64,     // l-64bit
    Ruyi_ir_type_Float32,   // f-32bit
    Ruyi_ir_type_Float64,   // d-64bit
    // reference types 64bit
    Ruyi_ir_type_Object,    // Oname;
    Ruyi_ir_type_String,    // S
    Ruyi_ir_type_Array,     // Atype
    Ruyi_ir_type_Tuple,     // Ttype*
    Ruyi_ir_type_Map,       // Mtypetype
    Ruyi_ir_type_Function,  // F(type*)type*
} ruyi_ir_type;

typedef enum {
    Ruyi_ir_Dup = 1,
    Ruyi_ir_Pop,        // pop and ignore the value
    Ruyi_ir_Push,       // push const to operators
    Ruyi_ir_Load,
    Ruyi_ir_Store,
    Ruyi_ir_Jmp = 20,
    Ruyi_ir_Je,
    Ruyi_ir_Jne,
    Ruyi_ir_Getglb = 40,
    Ruyi_ir_Setglb,
    Ruyi_ir_Iadd = 50,
    Ruyi_ir_Isub,
    Ruyi_ir_Idiv,
    Ruyi_ir_Imul,
    Ruyi_ir_Imod,
    Ruyi_ir_Iconst,
    Ruyi_ir_Iconst_0,
    Ruyi_ir_Iconst_1,
    Ruyi_ir_Iconst_m1,
    Ruyi_ir_Iinc,
    Ruyi_ir_Idec,
    Ruyi_ir_Iand,
    Ruyi_ir_Ior,
    Ruyi_ir_I_jgt,
    Ruyi_ir_I_jget,
    Ruyi_ir_I_jlt,
    Ruyi_ir_I_jlet,
    Ruyi_ir_I2f,
    Ruyi_ir_I2f_1,
    Ruyi_ir_Fadd = 100,
    Ruyi_ir_Fsub,
    Ruyi_ir_Fdiv,
    Ruyi_ir_Fmul,
    Ruyi_ir_Fconst,
    Ruyi_ir_Fconst_0,
    Ruyi_ir_Fconst_1,
    Ruyi_ir_Fconst_m1,
    Ruyi_ir_Finc,
    Ruyi_ir_Fdec,
    Ruyi_ir_F_jgt,
    Ruyi_ir_F_jget,
    Ruyi_ir_F_jlt,
    Ruyi_ir_F_jlet,
    Ruyi_ir_F2i,
    Ruyi_ir_F2i_1,
    
    Ruyi_ir_LeaGlb,
    Ruyi_ir_LeaLocal,
    Ruyi_ir_Getav,
    Ruyi_ir_Setav,
    Ruyi_ir_Invokesp = 150,
    Ruyi_ir_Invokenative,
    Ruyi_ir_Ret,
    //Ruyi_ir_Iret,
    Ruyi_ir_Fret,
} ruyi_ir_ins;

typedef struct {
    char    name[RUYI_IR_INS_NAME_LENGTH];
    BOOL    has_second; // flag it has or has not second argument, for example: with index or with offset etc.
    BOOL    may_jump;
    INT16   operand;
} ruyi_ir_ins_detail;

BOOL ruyi_ir_get_ins_detail(ruyi_ir_ins ins, ruyi_ir_ins_detail *ins_detail_out);

BOOL ruyi_ir_get_ins_code(const char *name, ruyi_ir_ins *ins_code_out);

UINT64 ruyi_ir_make_code(ruyi_ir_ins ins, UINT32 val);

void ruyi_ir_parse_code(UINT64 code, ruyi_ir_ins *ins_out, UINT32 *val_out);

BOOL ruyi_ir_code_desc(UINT64 code, char *name, UINT32 name_len, UINT32 *val_out, BOOL *has_second);

#endif /* ruyi_ir_h */
