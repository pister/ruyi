//
//  ruyi_code_generator.c
//  ruyi
//
//  Created by Songli Huang on 2019/11/7.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_code_generator.h"
#include "ruyi_mem.h"
#include "ruyi_error.h"
#include "ruyi_ast.h"
#include <string.h> // for memcpy

#define CG_FUNC_WRITE_CAP_INIT 16
#define CG_FILE_MAGIC_CODE 0xBBFFAA0098DAC699
#define CG_FILE_VERSION    0x01

#define PACKAGE_SEPARATE '.'

static void function_writer_destroy(ruyi_cg_function_writer *function_writer) {
    if (!function_writer) {
        return;
    }
    if (function_writer->codes) {
        ruyi_mem_free(function_writer->codes);
    }
    if (function_writer->name) {
        ruyi_unicode_string_destroy(function_writer->name);
    }
    ruyi_mem_free(function_writer);
}

static void function_writer_append_code(ruyi_cg_function_writer *writer, UINT64 code) {
    UINT32 new_capacity;
    UINT64 *new_codes;
    UINT64 *old_codes;
    while (writer->codes_length >= writer->codes_capacity) {
        new_capacity = (UINT32)(writer->codes_capacity * 1.5) + CG_FUNC_WRITE_CAP_INIT;
        new_codes = ruyi_mem_alloc(new_capacity * sizeof(UINT64));
        memcpy(new_codes, writer->codes, writer->codes_capacity * sizeof(UINT64));
        old_codes = writer->codes;
        writer->codes = new_codes;
        writer->codes_capacity = new_capacity;
        ruyi_mem_free(old_codes);
    }
    writer->codes[writer->codes_length] = code;
    writer->codes_length++;
}

static ruyi_error* function_writer_add_code(ruyi_cg_function_writer *writer, Ruyi_ir_ins ins, INT32 index, INT32 *seq_out) {
    ruyi_ir_ins_detail detail;
    UINT64 code;
    assert(writer);
    if (!ruyi_ir_get_ins_detail(ins, &detail)) {
        return ruyi_error_misc("can not find ins.");
    }
    if (detail.has_second) {
        code = ruyi_ir_make_code(ins, index);
    } else {
        code = ruyi_ir_make_code(ins, 0);
    }
    function_writer_append_code(writer, code);
    if (seq_out) {
        *seq_out = writer->seq;
    }
    writer->seq++;
    return NULL;
}

static ruyi_cg_ir_writer *ir_writer_create(void) {
    ruyi_cg_ir_writer *ir_writer = (ruyi_cg_ir_writer*)ruyi_mem_alloc(sizeof(ruyi_cg_ir_writer));
    // item type: <ruyi_cg_function_writer *>
    ir_writer->function_writers = ruyi_vector_create();
    // key-value type: <ruyi_unicode_string*, INT32>
    ir_writer->named_func_index = ruyi_hashtable_create();
    ir_writer->current_function_writer = NULL;
    return ir_writer;
}

static ruyi_error* ir_writer_write(ruyi_cg_ir_writer *writer, Ruyi_ir_ins ins, INT32 index, INT32 *seq_out) {
    assert(writer);
    assert(writer->current_function_writer);
    return function_writer_add_code(writer->current_function_writer, ins, index, seq_out);
}


static ruyi_error* ir_writer_write_jump(ruyi_cg_ir_writer *writer, Ruyi_ir_ins ins, INT32 offset, INT32 *seq_out) {
    assert(writer);
    assert(writer->current_function_writer);
    return function_writer_add_code(writer->current_function_writer, ins, writer->current_function_writer->seq + offset, seq_out);
}

static void ir_writer_set_second_value(ruyi_cg_ir_writer *writer, INT32 seq, UINT32 value) {
    Ruyi_ir_ins ins;
    assert(writer);
    assert(writer->current_function_writer);
    ruyi_ir_parse_code(writer->current_function_writer->codes[seq], &ins, NULL);
    writer->current_function_writer->codes[seq] = ruyi_ir_make_code(ins, value);
}

static INT32 ir_writer_get_code_seq(ruyi_cg_ir_writer *writer) {
    assert(writer);
    assert(writer->current_function_writer);
    return writer->current_function_writer->seq;
}

static void ir_writer_destroy(ruyi_cg_ir_writer *ir_writer) {
    ruyi_value temp;
    UINT32 i, len;
    if (!ir_writer) {
        return;
    }
    if (ir_writer->function_writers) {
        len = ruyi_vector_length(ir_writer->function_writers);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(ir_writer->function_writers, i, &temp);
            function_writer_destroy((ruyi_cg_function_writer *)temp.data.ptr);
        }
        ruyi_vector_destroy(ir_writer->function_writers);
    }
    if (ir_writer->named_func_index) {
        // the key of named_func_index is reference to name of item of function_writers,
        // so it will be released by function_writer_destroy(...);
        ruyi_hashtable_destroy(ir_writer->named_func_index);
    }
    ruyi_mem_free(ir_writer);
}

static
ruyi_cg_function_writer * ruyi_cg_define_function(ruyi_cg_ir_writer *ir_writer, const ruyi_unicode_string* name, INT32 index) {
    ruyi_cg_function_writer *function_writer;
    assert(ir_writer);
    function_writer = (ruyi_cg_function_writer*)ruyi_mem_alloc(sizeof(ruyi_cg_function_writer));
    function_writer->index = index;
    function_writer->arguments = 0;
    function_writer->codes_capacity = CG_FUNC_WRITE_CAP_INIT;
    function_writer->codes_length = 0;
    function_writer->codes = (UINT64*)ruyi_mem_alloc(CG_FUNC_WRITE_CAP_INIT * sizeof(UINT64));
    function_writer->locals = 0;
    function_writer->name = ruyi_unicode_string_copy_from(name);
    function_writer->operand = 0;
    function_writer->return_type = Ruyi_ir_type_Void;
    function_writer->ir_writer = ir_writer;
    ruyi_vector_add(ir_writer->function_writers, ruyi_value_ptr(function_writer));
    function_writer->ir_writer->current_function_writer = function_writer;
    ruyi_hashtable_put(ir_writer->named_func_index, ruyi_value_unicode_str(function_writer->name), ruyi_value_int32(index));
    return function_writer;
}

static
void cg_gen_func_define(ruyi_cg_function_writer *function_writer, ruyi_ast *func_define_ast)  {
    assert(Ruyi_at_function_declaration == func_define_ast->type);
    /*
     ast = ruyi_ast_create(Ruyi_at_function_declaration);
     ruyi_ast_add_child(ast, ast_name);
     ruyi_ast_add_child(ast, ast_formal_params);
     ruyi_ast_add_child(ast, ast_return_type);
     ruyi_ast_add_child(ast, ast_body);
     */
    
}

static void set_field_name_data(ruyi_cg_file *file, INT32 length_field_offset, INT32 name_field_offset, const ruyi_unicode_string* name) {
    ruyi_bytes_string *src_str;
    BYTE* ptr = (BYTE*)((void*)file);
    UINT16 *length_ptr = (UINT16*)(ptr + length_field_offset);
    BYTE **name_ptr = (BYTE**)(ptr + name_field_offset);
    src_str = ruyi_unicode_string_decode_utf8(name);
    *name_ptr = (BYTE*)ruyi_mem_alloc(src_str->length * sizeof(BYTE));
    *length_ptr = src_str->length;
    memcpy(*name_ptr, src_str->str, src_str->length * sizeof(BYTE));
    ruyi_unicode_bytes_string_destroy(src_str);
}

static ruyi_cg_file* create_ruyi_cg_file(const ruyi_unicode_string* name) {
    //ruyi_bytes_string *name_mstr;
    ruyi_cg_file* file;
    assert(name);
    file = (ruyi_cg_file*)ruyi_mem_alloc(sizeof(ruyi_cg_file));
    file->magic = CG_FILE_MAGIC_CODE;
    file->version = CG_FILE_VERSION;
    file->package_size = 0;
    file->package = NULL;
    set_field_name_data(file, RUYI_OFFSET_OF(ruyi_cg_file, name_size), RUYI_OFFSET_OF(ruyi_cg_file, name), name);
    file->cp_count = 0;
    file->cp = NULL;
    file->gv_count = 0;
    file->gv = NULL;
    file->func_count = 0;
    file->func = NULL;
    file->init_func_name_size = 0;
    file->init_func_name = NULL;
    file->entry_func_name_size = 0;
    file->entry_func_name = NULL;
    return file;
}


static void cp_destroy(ruyi_cg_file_const_pool *cp) {
    if (!cp) {
        return;
    }
    ruyi_mem_free(cp);
}

static void gv_destroy(ruyi_cg_file_global_var *gv) {
    if (!gv) {
        return;
    }
    ruyi_mem_free(gv);
}

static void func_destroy(ruyi_cg_file_function *func) {
    if (!func) {
        return;
    }
    if (func->name) {
        ruyi_mem_free(func->name);
    }
    if (func->codes) {
        ruyi_mem_free(func->codes);
    }
    ruyi_mem_free(func);
}

void ruyi_cg_file_destroy(ruyi_cg_file *file) {
    UINT16 i;
    if (!file) {
        return;
    }
    if (file->package) {
        ruyi_mem_free(file->package);
    }
    if (file->name) {
        ruyi_mem_free(file->name);
    }
    if (file->cp && file->cp_count > 0) {
        for (i = 0; i < file->cp_count; i++) {
            cp_destroy(file->cp[i]);
        }
    }
    if (file->gv && file->gv_count > 0) {
        for (i = 0; i < file->gv_count; i++) {
            gv_destroy(file->gv[i]);
        }
    }
    if (file->func && file->func_count > 0) {
        for (i = 0; i < file->func_count; i++) {
            func_destroy(file->func[i]);
        }
    }
    if (file->init_func_name) {
        ruyi_mem_free(file->init_func_name);
    }
    if (file->entry_func_name) {
        ruyi_mem_free(file->entry_func_name);
    }
    ruyi_mem_free(file);
}



static ruyi_error* gen_package(const ruyi_ast *ast, ruyi_cg_file *file) {
    ruyi_error *err;
    ruyi_ast *ast_name, *ast_sub_name;
    UINT32 i, len;
    ruyi_unicode_string *package_name = NULL;
    if (!ast) {
        return NULL;
    }
    if (Ruyi_at_package_declaration != ast->type) {
        err = ruyi_error_misc("need package declaration ast");
        goto gen_package_on_error;
    }
    if (1 != ruyi_ast_child_length(ast)) {
        err = ruyi_error_misc("child length of package declaration ast must be 1");
        goto gen_package_on_error;
    }
    ast_name = ruyi_ast_get_child(ast, 0);
    if (Ruyi_at_name != ast_name->type) {
        err = ruyi_error_misc("child of package declaration ast must be name");
        goto gen_package_on_error;
    }
    if (Ruyi_adt_unicode_str != ast_name->adt_type) {
        err = ruyi_error_misc("type of package declaration ast must be unicode string");
        goto gen_package_on_error;
    }
    package_name = ruyi_unicode_string_copy_from((ruyi_unicode_string*)ast_name->data.ptr_value);
    /*
     name: part0 child[part1, part2, part3, ...]
     */
    for ((void)(i = 0), len = ruyi_ast_child_length(ast_name); i < len; i++) {
        ast_sub_name = ruyi_ast_get_child(ast_name, i);
        if (Ruyi_at_name_part != ast_sub_name->type) {
            err = ruyi_error_misc("child of package declaration ast must be name");
            goto gen_package_on_error;
        }
        if (Ruyi_adt_unicode_str != ast_sub_name->adt_type) {
            err = ruyi_error_misc("type of package declaration ast must be unicode string");
            goto gen_package_on_error;
        }
        ruyi_unicode_string_append_wide_char(package_name, PACKAGE_SEPARATE);
        ruyi_unicode_string_append_unicode(package_name, (ruyi_unicode_string*)ast_sub_name->data.ptr_value);
    }
    set_field_name_data(file, RUYI_OFFSET_OF(ruyi_cg_file, package_size), RUYI_OFFSET_OF(ruyi_cg_file, package), package_name);
    ruyi_unicode_string_destroy(package_name);
    return NULL;
gen_package_on_error:
    if (package_name) {
        ruyi_unicode_string_destroy(package_name);
    }
    return err;
}

static ruyi_error* gen_import(const ruyi_ast *ast, ruyi_cg_file *file) {
    
    return NULL;
}

static ruyi_error* gen_global(const ruyi_ast *ast, ruyi_cg_file *file) {
    
    return NULL;
}

ruyi_error* ruyi_cg_generate(const ruyi_ast *ast, ruyi_cg_file **out_ir_file) {
    ruyi_error *err;
    ruyi_cg_file *file = NULL;
    ruyi_unicode_string *name;
    ruyi_ast *ast_package, *ast_import, *ast_global;
    assert(ast);
    name = ruyi_unicode_string_init_from_utf8("test1", 0);
    file = create_ruyi_cg_file(name);
    ruyi_unicode_string_destroy(name);
    if (Ruyi_at_root != ast->type) {
        err = ruyi_error_misc("need root ast");
        goto ruyi_cg_generate_on_error;
    }
    ast_package = ruyi_ast_get_child(ast, 0);
    ast_import = ruyi_ast_get_child(ast, 1);
    ast_global = ruyi_ast_get_child(ast, 2);
    if ((err = gen_package(ast_package, file)) != NULL) {
        goto ruyi_cg_generate_on_error;
    }
    if ((err = gen_import(ast_import, file)) != NULL) {
        goto ruyi_cg_generate_on_error;
    }
    if ((err = gen_global(ast_global, file)) != NULL) {
        goto ruyi_cg_generate_on_error;
    }
    *out_ir_file = file;
    return NULL;
ruyi_cg_generate_on_error:
    if (file) {
        ruyi_cg_file_destroy(file);
    }
    *out_ir_file = NULL;
    return err;
}
