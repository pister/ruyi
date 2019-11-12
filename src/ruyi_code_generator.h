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
#include "ruyi_vector.h"
#include "ruyi_hashtable.h"
#include "ruyi_error.h"


typedef enum {
    Ruyi_cpt_class,
    Ruyi_cpt_field_ref,
    Ruyi_cpt_method_ref,
    Ruyi_cpt_interface_method_ref,
    Ruyi_cpt_utf8,
    Ruyi_cpt_string,
    Ruyi_cpt_int32,
    Ruyi_cpt_float32,
    Ruyi_cpt_int64,
    Ruyi_cpt_float64,
    Ruyi_cpt_name_and_type,
    Ruyi_cpt_method_handle,
    Ruyi_cpt_method_type,
    Ruyi_cpt_invoke_dynamic,
} Ruyi_cp_type;

struct ruyi_cg_ir_writer_;

typedef struct {
    ruyi_unicode_string *name;
    INT32 index;
    ruyi_ir_type return_type;
    UINT16 operand;
    UINT16 locals;
    UINT16 arguments;
    UINT64 *codes;
    UINT32 codes_length;
    UINT32 codes_capacity;
    INT32 seq;
    struct ruyi_cg_ir_writer_ *ir_writer;
} ruyi_cg_function_writer;

typedef struct ruyi_cg_ir_writer_ {
    ruyi_symtab *symtab;
    ruyi_vector *function_writers;
    ruyi_hashtable *named_func_index;
    ruyi_cg_function_writer *current_function_writer;
} ruyi_cg_ir_writer;

typedef struct {
    Ruyi_cp_type    type;
    UINT16          index;
    UINT16          value_size;
    BYTE*           value;
} ruyi_cg_file_const_pool;

typedef struct {
    ruyi_ir_type    type;
    UINT32          var_size;
    UINT16          index;
    UINT16          name_size;
    BYTE*           name;
} ruyi_cg_file_global_var;

typedef struct {
    ruyi_ir_type    return_type;
    UINT16          index;
    UINT16          name_size;
    BYTE            *name;
    UINT16          oparand;
    UINT16          local_size;
    UINT16          argument_size;
    UINT32          codes_count;
    UINT64          *codes;
} ruyi_cg_file_function;


typedef struct {
    UINT64                  magic;
    UINT32                  version;
    UINT16                  package_size;
    BYTE                    *package;
    UINT16                  name_size;
    BYTE                    *name;
    UINT16                  cp_count;
    ruyi_cg_file_const_pool **cp;
    UINT16                  gv_count;
    ruyi_cg_file_global_var **gv;
    UINT16                  func_count;
    ruyi_cg_file_function   **func;
    UINT16                  init_func_name_size;
    BYTE                    *init_func_name;
    UINT16                  entry_func_name_size;
    BYTE                    *entry_func_name;
} ruyi_cg_file;

ruyi_error* ruyi_cg_generate(const ruyi_ast *ast, ruyi_cg_file **out_ir_file);

void ruyi_cg_file_destroy(ruyi_cg_file *ir_file);


#endif /* ruyi_code_generator_h */
