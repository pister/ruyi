//
//  ruyi_code_generator.c
//  ruyi
//
//  Created by Songli Huang on 2019/11/7.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_code_generator.h"
#include "ruyi_mem.h"




/*
 func (fw *FunctionWriter) AddCode(ins vm.Ins, index int) (int, error) {
 i, exist := vm.GetInsDetail(ins)
 if !exist {
 return 0, fmt.Errorf("not support ins: %d", ins)
 }
 if i.OperatorValue {
 fw.codes = append(fw.codes, vm.MakeCode(ins, uint32(index)))
 } else {
 fw.codes = append(fw.codes, vm.MakeCode(ins, 0))
 }
 codeSeq := fw.seq
 fw.seq++
 return codeSeq, nil
 }
 */

// ==========

static ruyi_cg_function_writer *function_writer_create(void) {
    // TODO
    return NULL;
}

static void function_writer_destroy(ruyi_cg_function_writer *writer) {
    // TODO
}

static void function_writer_add_code(ruyi_cg_function_writer *writer, Ruyi_ir_ins ins, INT32 index) {
    
}

// ==========
static ruyi_cg_ir_writer *ir_writer_create(void) {
    ruyi_cg_ir_writer *ir_writer = (ruyi_cg_ir_writer*)ruyi_mem_alloc(sizeof(ruyi_cg_ir_writer));
    // TODO init members
    
    return ir_writer;
}

static void ir_writer_destroy(ruyi_cg_ir_writer *ir_writer) {
    if (!ir_writer) {
        return;
    }
    // TODO destroy members
    
    ruyi_mem_free(ir_writer);
}

static INT32 ir_write(ruyi_cg_ir_writer *ir_writer, Ruyi_ir_ins ins_code, INT32 index) {
    // TODO
    return 0;
}

static INT32 ir_write_jump(ruyi_cg_ir_writer *ir_writer, Ruyi_ir_ins ins_code, INT32 offset) {
    // TODO
    return 0;
}


//

ruyi_code_generator *ruyi_cg_create(void) {
    ruyi_code_generator *cg = (ruyi_code_generator*)ruyi_mem_alloc(sizeof(ruyi_code_generator));
    cg->ir_writer = ir_writer_create();
    cg->symtab = ruyi_symtab_create();
    return cg;
}

void ruyi_cg_destroy(ruyi_code_generator *cg) {
    if (!cg) {
        return;
    }
    ruyi_symtab_destroy(cg->symtab);
    ir_writer_destroy(cg->ir_writer);
    ruyi_mem_free(cg);
}

void ruyi_cg_write(ruyi_code_generator *cg, Ruyi_ir_ins ins_code, INT32 index) {
    assert(cg);
    ir_write(cg->ir_writer, ins_code, index);
}

INT32 ruyi_cg_write_jump(ruyi_code_generator *cg, Ruyi_ir_ins ins_code, INT32 offset) {
    assert(cg);
    return ir_write_jump(cg->ir_writer, ins_code, offset);
}
