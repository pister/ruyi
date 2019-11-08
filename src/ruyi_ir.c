//
//  ruyi_ir.c
//  ruyi
//
//  Created by Songli Huang on 2019/11/7.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_ir.h"
#include "ruyi_hashtable.h"
#include "ruyi_mem.h"
#include <string.h> // for memcpy

static ruyi_hashtable *g_ins_table = NULL;
static ruyi_hashtable *g_ins_name_table = NULL;

static void put_ins_detail(ruyi_hashtable *ins_table, ruyi_hashtable *ins_name_table, Ruyi_ir_ins ins, const char* name, BOOL has_second, BOOL may_jump, INT16 operand) {
    assert(strlen(name) < RUYI_IR_INS_NAME_LENGTH - 1);
    ruyi_ir_ins_detail *temp = ruyi_mem_alloc(sizeof(ruyi_ir_ins_detail));
    strncpy(temp->name, name, RUYI_IR_INS_NAME_LENGTH);
    temp->has_second = has_second;
    temp->may_jump = may_jump;
    temp->operand = operand;
    ruyi_hashtable_put(ins_table, ruyi_value_int32(ins), ruyi_value_ptr(temp));
    ruyi_hashtable_put(ins_name_table, ruyi_value_str(temp->name), ruyi_value_int32(ins));
}

static void init_ins_tables() {
    g_ins_table = ruyi_hashtable_create();
    g_ins_name_table = ruyi_hashtable_create();
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Dup, "dup", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Pop, "pop", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Push, "push", TRUE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Push, "load", TRUE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Store, "store", TRUE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Jmp, "jmp", TRUE, TRUE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Je, "je", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Jne, "jne", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_I_jgt, "i_jgt", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_I_jget, "i_jget", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_I_jlt, "i_jlt", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_I_jlet, "i_jlet", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Getglb, "getglb", TRUE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Setglb, "setglb", TRUE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iadd, "iadd", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Isub, "isub", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Idiv, "idiv", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Imul, "imul", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Imod, "imod", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iconst, "iconst", TRUE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iconst_0, "iconst_0", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iconst_1, "iconst_1", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iconst_2, "iconst_2", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iconst_m1, "iconst_m1", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iinc, "iinc", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Idec, "idec", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fadd, "fadd", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fsub, "fsub", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fmul, "fmul", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fdiv, "fdiv", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fconst_0, "fconst_0", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fconst_1, "fconst_1", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fconst_m1, "fconst_m1", FALSE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Finc, "finc", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fdec, "fdec", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_F_jgt, "f_jgt", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_F_jget, "f_jget", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_F_jlt, "f_jlt", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_F_jlet, "f_jlet", TRUE, TRUE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iand, "iand", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Ior, "ior", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_I2f, "i2f", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_F2i, "f2i", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_I2f_1, "f2i_1", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_F2i_1, "f2i_1", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_LeaGlb, "leaglb", TRUE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_LeaLocal, "lealocal", TRUE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Getav, "getav", TRUE, FALSE, 1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Setav, "setav", TRUE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Invokesp, "invokesp", TRUE, TRUE, 0); // operand count dependency on arguments
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Invokenative, "invokenative", TRUE, TRUE, 0); // operand count dependency on arguments
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Ret, "ret", FALSE, FALSE, 0);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Iret, "iret", FALSE, FALSE, -1);
    put_ins_detail(g_ins_table, g_ins_name_table, Ruyi_ir_Fret, "fret", FALSE, FALSE, -1);

}

BOOL ruyi_ir_get_ins_detail(Ruyi_ir_ins ins, ruyi_ir_ins_detail *ins_detail_out) {
    ruyi_value value;
    ruyi_ir_ins_detail *found_value;
    if (!g_ins_table) {
        // init
        init_ins_tables();
    }
    if (!ins_detail_out) {
        return FALSE;
    }
    if (!ruyi_hashtable_get(g_ins_table, ruyi_value_int32(ins), &value)) {
        return FALSE;
    }
    found_value = (ruyi_ir_ins_detail*)value.data.ptr;
    if (!found_value) {
        // may not reach here
        return FALSE;
    }
    memcpy(ins_detail_out, found_value, sizeof(ruyi_ir_ins_detail));
    return TRUE;
}

BOOL ruyi_ir_get_ins_code(const char *name, Ruyi_ir_ins *ins_code_out) {
    ruyi_value value;
    if (!g_ins_table) {
        init_ins_tables();
    }
    if (!name) {
        return FALSE;
    }
    if (!ruyi_hashtable_get(g_ins_name_table, ruyi_value_str(name), &value)) {
        return FALSE;
    }
    *ins_code_out = (Ruyi_ir_ins)value.data.int32_value;
    return TRUE;
}

UINT64 ruyi_ir_make_code(Ruyi_ir_ins ins, UINT32 val) {
    UINT64 code = (UINT64)ins;
    code = code << 32;
    code = code | (0x00000000ffffffff & (UINT64)(val));
    return code;
}

void ruyi_ir_parse_code(UINT64 code, Ruyi_ir_ins *ins_out, UINT32 *val_out) {
    if (ins_out) {
        *ins_out = (UINT32)(code >> 32);
    }
    if (val_out) {
        *val_out = (UINT32)(0x00000000ffffffff & code);
    }
}
