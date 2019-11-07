//
//  ruyi_code_generator.h
//  ruyi
//
//  Created by Songli Huang on 2019/11/7.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_code_generator_h
#define ruyi_code_generator_h

#include "ruyi_symtab.h"
#include "ruyi_ast.h"
#include "ruyi_ir.h"

typedef struct {
    ruyi_unicode_string *name;
    Ruyi_ir_type return_type;
    UINT16 operand;
    UINT16 locals;
    UINT16 arguments;
    UINT64 *codes;
    INT32 seq;
} ruyi_cg_function_writer;

typedef struct {
    
} ruyi_cg_ir_writer;

typedef struct {
    ruyi_symtab *symtab;
    ruyi_cg_ir_writer *ir_writer;
} ruyi_code_generator;

ruyi_code_generator *ruyi_cg_create(void);

void ruyi_cg_destroy(ruyi_code_generator *cg);

void ruyi_cg_write(ruyi_code_generator *cg, Ruyi_ir_ins ins_code, INT32 index);

INT32 ruyi_cg_write_jump(ruyi_code_generator *cg, Ruyi_ir_ins ins_code, INT32 offset);

#endif /* ruyi_code_generator_h */
