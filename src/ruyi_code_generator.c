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
#include "ruyi_vector.h"
#include "ruyi_symtab.h"
#include <string.h> // for memcpy

#define CG_FUNC_WRITE_CAP_INIT 16
#define CG_FILE_MAGIC_CODE 0xBBFFAA0098DAC699
#define CG_FILE_VERSION    0x01

#define PACKAGE_SEPARATE '.'

#define NAME_BUF_LENGTH 128

static ruyi_error* handle_type(ruyi_ast *ast_type, ruyi_symtab_type *out_type);

static ruyi_error* make_unicode_name_error(const char* fmt, const ruyi_unicode_string *name) {
    char temp_name[NAME_BUF_LENGTH];
    ruyi_unicode_string_encode_utf8_n(name, temp_name, NAME_BUF_LENGTH-1);
    return ruyi_error_misc(fmt, temp_name);
}

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

static ruyi_error* function_writer_add_code(ruyi_cg_function_writer *writer, ruyi_ir_ins ins, INT32 index, INT32 *seq_out) {
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

static ruyi_error* ir_writer_write(ruyi_cg_ir_writer *writer, ruyi_ir_ins ins, INT32 index, INT32 *seq_out) {
    assert(writer);
    assert(writer->current_function_writer);
    return function_writer_add_code(writer->current_function_writer, ins, index, seq_out);
}


static ruyi_error* ir_writer_write_jump(ruyi_cg_ir_writer *writer, ruyi_ir_ins ins, INT32 offset, INT32 *seq_out) {
    assert(writer);
    assert(writer->current_function_writer);
    return function_writer_add_code(writer->current_function_writer, ins, writer->current_function_writer->seq + offset, seq_out);
}

static void ir_writer_set_second_value(ruyi_cg_ir_writer *writer, INT32 seq, UINT32 value) {
    ruyi_ir_ins ins;
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
    src_str = ruyi_unicode_string_encode_utf8(name);
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
    // TODO free value ?
    ruyi_mem_free(cp);
}

static UINT32 copy_unicode_to_bytes(const ruyi_unicode_string * s, BYTE **dest, UINT16 *dest_len) {
    ruyi_bytes_string *temp;
    if (s == NULL) {
        return 0;
    }
    temp = ruyi_unicode_string_encode_utf8(s);
    if (dest_len) {
        *dest_len = (UINT16)temp->length;
    }
    *dest = (BYTE *)ruyi_mem_alloc(temp->length);
    memcpy(*dest, temp->str, temp->length);
    ruyi_unicode_bytes_string_destroy(temp);
    return temp->length;
}

static ruyi_cg_file_global_var* gv_create(const ruyi_symtab_variable *symtab_var) {
    ruyi_cg_file_global_var *gv = (ruyi_cg_file_global_var*)ruyi_mem_alloc(sizeof(ruyi_cg_file_global_var));
    gv->index = symtab_var->index;
    gv->type = symtab_var->type.ir_type;
    gv->var_size = symtab_var->type.size;
    copy_unicode_to_bytes(symtab_var->name, &gv->name, &gv->name_size);
    return gv;
}

static void gv_destroy(ruyi_cg_file_global_var *gv) {
    if (!gv) {
        return;
    }
    if (gv->name) {
        ruyi_mem_free(gv->name);
    }
    ruyi_mem_free(gv);
}

static ruyi_cg_file_function* func_create(const ruyi_symtab_function_define *symtab_func) {
    UINT32 i, len;
    ruyi_value temp_value;
    ruyi_ir_type ir_type;
    ruyi_cg_file_function *func = (ruyi_cg_file_function*)ruyi_mem_alloc(sizeof(ruyi_cg_file_function));
    func->index = symtab_func->index;
    copy_unicode_to_bytes(symtab_func->name, &func->name, &func->name_size);
    // return types
    len = ruyi_vector_length(symtab_func->return_types);
    // why the size div by 2 ?
    // see ruyi_symtab_function_add_return_type(...)
    func->return_size = (UINT16)len/2;
    if (func->return_size > 0) {
        func->return_types = (ruyi_ir_type*)ruyi_mem_alloc(sizeof(ruyi_ir_type*) * func->return_size);
        for (i = 0; i < len; i += 2) {
            // ir_type
            ruyi_vector_get(symtab_func->return_types, i, &temp_value);
            ir_type = temp_value.data.int32_value;
            func->return_types[i/2] = ir_type;
            // ignore detail
            // ruyi_vector_get(symtab_func->return_types, i + 1, &temp_value);
        }
    } else {
        func->return_types = NULL;
    }
    
    // arguments
    len = ruyi_vector_length(symtab_func->args_types);
    // why the size div by 3 ?
    // see ruyi_symtab_function_add_arg(...)
    func->argument_size = len/3;
    if (func->argument_size > 0) {
        func->argument_types = (ruyi_ir_type*)ruyi_mem_alloc(sizeof(ruyi_ir_type*) * func->argument_size);
        for (i = 0; i < len; i += 3) {
            // ignore name
            // ruyi_vector_get(symtab_func->args_types, i, &temp_value);

            // type
            ruyi_vector_get(symtab_func->args_types, i + 1, &temp_value);
            ir_type = temp_value.data.int32_value;
            func->argument_types[i/3] = ir_type;

            // ignore detail
            // ruyi_vector_get(symtab_func->args_types, i + 2, &temp_value);
        }
    }
    
    // codes
    func->codes_size = symtab_func->codes_size;
    if (func->codes_size > 0) {
        func->codes = (UINT64 *)ruyi_mem_alloc(sizeof(UINT64) * func->codes_size);
        memcpy(func->codes, symtab_func->codes, sizeof(UINT64) * func->codes_size);
    } else {
        func->codes = NULL;
    }
    
    
    // TODO
    return func;
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

void ruyi_cg_file_destroy(ruyi_cg_file *ir_file) {
    UINT16 i;
    if (!ir_file) {
        return;
    }
    if (ir_file->package) {
        ruyi_mem_free(ir_file->package);
    }
    if (ir_file->name) {
        ruyi_mem_free(ir_file->name);
    }
    if (ir_file->cp && ir_file->cp_count > 0) {
        for (i = 0; i < ir_file->cp_count; i++) {
            cp_destroy(ir_file->cp[i]);
        }
        ruyi_mem_free(ir_file->cp);
    }
    if (ir_file->gv && ir_file->gv_count > 0) {
        for (i = 0; i < ir_file->gv_count; i++) {
            gv_destroy(ir_file->gv[i]);
        }
        ruyi_mem_free(ir_file->gv);
    }
    if (ir_file->func && ir_file->func_count > 0) {
        for (i = 0; i < ir_file->func_count; i++) {
            func_destroy(ir_file->func[i]);
        }
        ruyi_mem_free(ir_file->func);
    }
    if (ir_file->init_func_name) {
        ruyi_mem_free(ir_file->init_func_name);
    }
    if (ir_file->entry_func_name) {
        ruyi_mem_free(ir_file->entry_func_name);
    }
    ruyi_mem_free(ir_file);
}

static ruyi_error* gen_package(ruyi_symtab *symtab, const ruyi_ast *ast, ruyi_cg_file *ir_file) {
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
    len = ruyi_ast_child_length(ast_name);
    for (i = 0; i < len; i++) {
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
    set_field_name_data(ir_file, RUYI_OFFSET_OF(ruyi_cg_file, package_size), RUYI_OFFSET_OF(ruyi_cg_file, package), package_name);
    ruyi_unicode_string_destroy(package_name);
    return NULL;
gen_package_on_error:
    if (package_name) {
        ruyi_unicode_string_destroy(package_name);
    }
    return err;
}

static ruyi_error* gen_import(ruyi_symtab *symtab, const ruyi_ast *ast, ruyi_cg_file *ir_file) {
    // TODO
    return NULL;
}

static BOOL type_can_assign(const ruyi_symtab_type* dest, const ruyi_symtab_type* src) {
    // TODO
    return TRUE;
}

static ruyi_error* gen_global_var_define(ruyi_symtab *symtab, const ruyi_ast *ast, ruyi_vector *global_vars) {
    ruyi_error *err;
    ruyi_symtab_variable var;
    BOOL has_init_expr = FALSE;
    ruyi_symtab_type expr_type;
    ruyi_symtab_type var_decleration_type;
    assert(ast);
    assert(Ruyi_at_var_declaration == ast->type);
    var.name = (ruyi_unicode_string*)ast->data.ptr_value;
    if (ruyi_ast_child_length(ast) >= 2) {
        has_init_expr = TRUE;
    }
    if (Ruyi_at_var_declaration_auto_type == ruyi_ast_get_child(ast, 0)->type) {
        if (!has_init_expr) {
            err = make_unicode_name_error("miss initialize expression for auto-type when define global var: %s", var.name);
            goto gen_global_var_define_on_error;
        }
        if ((err = handle_type(ruyi_ast_get_child(ast, 1), &expr_type)) != NULL) {
            goto gen_global_var_define_on_error;
        }
        // TODO need to generate init code and auto-type-cast ir at <init> func: for ast_init_expr.
        var.type = expr_type;
    } else {
        if ((err = handle_type(ruyi_ast_get_child(ast, 0), &var_decleration_type)) != NULL) {
            goto gen_global_var_define_on_error;
        }
        if (has_init_expr) {
            if ((err = handle_type(ruyi_ast_get_child(ast, 1), &expr_type)) != NULL) {
                goto gen_global_var_define_on_error;
            }
            if (!type_can_assign(&expr_type, &var_decleration_type)) {
                err = make_unicode_name_error("var can not be assigned by diference type when define global var: %s", var.name);
                goto gen_global_var_define_on_error;
            }
            // TODO need to generate init code and auto-type-cast ir at <init> func: for ast_init_expr.
        }
        var.type = var_decleration_type;
    }
    var.scope_type = Ruyi_sst_Global;
    if (( err = ruyi_symtab_add_global_var(symtab, &var, &var.index)) != NULL) {
        goto gen_global_var_define_on_error;
    }
    ruyi_vector_add(global_vars, ruyi_value_ptr(gv_create(&var)));
    // TODO free var ???
    return NULL;
gen_global_var_define_on_error:
    return err;
}


static ruyi_error* handle_type_array(ruyi_ast *ast_type, ruyi_symtab_type *out_type) {
    ruyi_error *err;
    ruyi_symtab_type raw_type;
    UINT16 dims = 0;
    ruyi_ast *temp_type = ast_type;
    assert(out_type);
    while (Ruyi_at_type_array == temp_type->type) {
        dims++;
        if (0 == ruyi_ast_child_length(temp_type)) {
            break;
        }
        temp_type = ruyi_ast_get_child(temp_type, 0);
    }
    if ((err = handle_type(temp_type, &raw_type)) != NULL) {
        return err;
    }
    out_type->ir_type = Ruyi_ir_type_Array;
    out_type->detail.array = ruyi_symtab_type_array_create(dims, raw_type);
    return NULL;
}

static ruyi_error* handle_type(ruyi_ast *ast_type, ruyi_symtab_type *out_type) {
    ruyi_error *err;
    ruyi_symtab_type_array *array;
    ruyi_symtab_type_array *func;
    ruyi_symtab_type_array *map;
    ruyi_symtab_type temp;
    assert(ast_type);
    assert(out_type);
    /*
     Ruyi_at_type_array,
     Ruyi_at_type_map,
     Ruyi_at_type_func,
     */
    out_type->detail.uniptr = NULL;
    switch (ast_type->type) {
        case Ruyi_at_type_byte:
            out_type->ir_type = Ruyi_ir_type_Byte;
            out_type->size = 1;
            break;
        case Ruyi_at_type_bool:
            out_type->ir_type = Ruyi_ir_type_Byte;
            out_type->size = 1;
            break;
        case Ruyi_at_type_short:
            out_type->ir_type = Ruyi_ir_type_Int16;
            out_type->size = 2;
            break;
        case Ruyi_at_integer:
        case Ruyi_at_type_int:
            out_type->ir_type = Ruyi_ir_type_Int32;
            out_type->size = 4;
            break;
        case Ruyi_at_type_rune:
            out_type->ir_type = Ruyi_ir_type_Rune;
            out_type->size = 4;
            break;
        case Ruyi_at_type_long:
            out_type->ir_type = Ruyi_ir_type_Int64;
            out_type->size = 8;
            break;
        case Ruyi_at_type_float:
            out_type->ir_type = Ruyi_ir_type_Float32;
            out_type->size = 4;
            break;
        case Ruyi_at_float:
        case Ruyi_at_type_double:
            out_type->ir_type = Ruyi_ir_type_Float64;
            out_type->size = 8;
            break;
        case Ruyi_at_type_array:
            err = handle_type_array(ast_type, out_type);
            if (err != NULL) {
                return err;
            }
            break;
            
            // TODO not finish
        default:
            return ruyi_error_syntax("unknown type support.");
    }
    
    return NULL;
}

typedef struct {
    UINT64 *data;
    UINT32 len;
    UINT32 cap;
} ruyi_ins_codes;

typedef struct {
    ruyi_symtab_function_define *func;
    ruyi_ins_codes              *codes;
    ruyi_symtab                 *symtab;    // reference of global ruyi_symtab
} ruyi_cg_body_context;

static
ruyi_ins_codes* ruyi_ins_codes_create() {
    const UINT32 init_cap = 32;
    ruyi_ins_codes *codes = (ruyi_ins_codes*)ruyi_mem_alloc(sizeof(UINT64) * init_cap);
    codes->cap = init_cap;
    codes->len = 0;
    codes->data = (UINT64*)ruyi_mem_alloc(sizeof(UINT64) * codes->cap);
    return codes;
}

static
void ruyi_ins_codes_destroy(ruyi_ins_codes *codes) {
    if (!codes) {
        return;
    }
    if (codes->data) {
        ruyi_mem_free(codes->data);
        codes->data = NULL;
    }
    ruyi_mem_free(codes);
}

static
UINT32 ruyi_ins_codes_add(ruyi_ins_codes *codes, ruyi_ir_ins ins, UINT32 val) {
    UINT32 pos;
    while (codes->len >= codes->cap) {
        UINT32 new_cap = (UINT32)(codes->cap * 1.5 + 1);
        UINT64 *new_data = (UINT64*)ruyi_mem_alloc(sizeof(UINT64) * new_cap);
        memcpy(new_data, codes->data, sizeof(UINT64) * codes->cap);
        ruyi_mem_free(codes->data);
        codes->data = new_data;
        codes->cap = new_cap;
    }
    pos = codes->len;
    codes->data[codes->len++] = ruyi_ir_make_code(ins, val);
    return pos;
}


static ruyi_error* gen_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type);


static ruyi_error* gen_return_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type) {
    ruyi_error *err;
    ruyi_ast *return_expr_list_ast;
    UINT32 len = 0;
    UINT32 i;
    if (ruyi_ast_child_length(ast_stmt) > 0) {
        return_expr_list_ast = ruyi_ast_get_child(ast_stmt, 0);
        len = ruyi_ast_child_length(return_expr_list_ast);
        for (i = 0; i < len; i++) {
            if ((err = gen_stmt(context, ruyi_ast_get_child(return_expr_list_ast, i), out_type, NULL)) != NULL ) {
                return err;
            }
        }
    }
    ruyi_ins_codes_add(context->codes, Ruyi_ir_Ret, len);
    return NULL;
}

static ruyi_error* gen_binary_expr_with_cast(const ruyi_symtab_type *left_type, const ruyi_symtab_type *right_type, ruyi_symtab_type *out_type, const ruyi_ast *op,
                   ruyi_ins_codes *codes, const ruyi_ast_type* ops, const ruyi_ir_ins *int64_ins, const ruyi_ir_ins *double_ins, UINT32 len) {
    int i;
    switch (left_type->ir_type) {
        case Ruyi_ir_type_Byte:
        case Ruyi_ir_type_Int16:
        case Ruyi_ir_type_Int32:
        case Ruyi_ir_type_Rune:
        case Ruyi_ir_type_Int64:
            if (Ruyi_ir_type_Int64 == right_type->ir_type) {
                for (i = 0; i < len; i++) {
                    if (ops[i] == op->type) {
                        ruyi_ins_codes_add(codes, int64_ins[i], 0);
                        break;
                    }
                }
                if (out_type) {
                    out_type->ir_type = Ruyi_ir_type_Int64;
                    out_type->detail.uniptr = NULL;
                    out_type->size = 8;
                }
            } else if (Ruyi_ir_type_Float64 == right_type->ir_type){
                ruyi_ins_codes_add(codes, Ruyi_ir_I2f_1, 0);
                for (i = 0; i < len; i++) {
                    if (ops[i] == op->type) {
                        ruyi_ins_codes_add(codes, double_ins[i], 0);
                        break;
                    }
                }
                if (out_type) {
                    out_type->ir_type = Ruyi_ir_type_Float64;
                    out_type->detail.uniptr = NULL;
                    out_type->size = 8;
                }
            } else {
                return ruyi_error_misc("unsupport cast type to int64");
            }
            break;
        case Ruyi_ir_type_Float32:
        case Ruyi_ir_type_Float64:
            if (Ruyi_ir_type_Int64 == right_type->ir_type){
                ruyi_ins_codes_add(codes, Ruyi_ir_I2f, 0);
            }
            for (i = 0; i < len; i++) {
                if (ops[i] == op->type) {
                    ruyi_ins_codes_add(codes, double_ins[i], 0);
                    break;
                }
            }
            if (out_type) {
                out_type->ir_type = Ruyi_ir_type_Float64;
                out_type->detail.uniptr = NULL;
                out_type->size = 8;
            }
            break;
        default:
            return ruyi_error_misc("unknown left type: %d", left_type);
    }
    return NULL;
}

static ruyi_error* gen_additive_expression(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err;
    UINT32 len = ruyi_ast_child_length(ast_stmt);
    ruyi_ast *left;
    ruyi_ast *right;
    ruyi_ast *op;
    ruyi_symtab_type left_type, right_type;
    const ruyi_ast_type ops[2] = {Ruyi_at_op_add, Ruyi_at_op_sub};
    const ruyi_ir_ins int64_ins[2] = {Ruyi_ir_Iadd, Ruyi_ir_Isub};
    const ruyi_ir_ins double_ins[2] = {Ruyi_ir_Fadd, Ruyi_ir_Fsub};
    assert(3 == len);
    left = ruyi_ast_get_child(ast_stmt, 0);
    op = ruyi_ast_get_child(ast_stmt, 1);
    right = ruyi_ast_get_child(ast_stmt, 2);
    if ((err = gen_stmt(context, left, &left_type, NULL)) != NULL) {
        return err;
    }
    if ((err = gen_stmt(context, right, &right_type, &left_type)) != NULL) {
        return err;
    }
    return gen_binary_expr_with_cast(&left_type, &right_type, out_type, op, context->codes, ops, int64_ins, double_ins, sizeof(ops)/sizeof(ops[0]));
}

static ruyi_error* gen_multiplicative_expression(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err;
    UINT32 len = ruyi_ast_child_length(ast_stmt);
    ruyi_ast *left;
    ruyi_ast *right;
    ruyi_ast *op;
    ruyi_symtab_type left_type, right_type;
    const ruyi_ast_type ops[3] = {Ruyi_at_op_mul, Ruyi_at_op_div, Ruyi_at_op_mod};
    const ruyi_ir_ins int64_ins[3] = {Ruyi_ir_Imul, Ruyi_ir_Idiv, Ruyi_ir_Imod};
    const ruyi_ir_ins double_ins[3] = {Ruyi_ir_Fadd, Ruyi_ir_Fsub, 0};
    assert(3 == len);
    left = ruyi_ast_get_child(ast_stmt, 0);
    op = ruyi_ast_get_child(ast_stmt, 1);
    right = ruyi_ast_get_child(ast_stmt, 2);
    if ((err = gen_stmt(context, left, &left_type, NULL)) != NULL) {
        return err;
    }
    if ((err = gen_stmt(context, right, &right_type, &left_type)) != NULL) {
        return err;
    }
    return gen_binary_expr_with_cast(&left_type, &right_type, out_type, op, context->codes, ops, int64_ins, double_ins, sizeof(ops)/sizeof(ops[0]));
}

static ruyi_error* gen_load_from_variable_name(ruyi_cg_body_context *context, const ruyi_unicode_string *name, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    UINT32 index;
    char name_buf[NAME_BUF_LENGTH];
    ruyi_symtab_variable var;
    if (ruyi_symtab_function_scope_get(context->func->func_symtab_scope, name, &var)) {
        // load from local
        index = var.index;
        if (out_type) {
            *out_type = var.type;
        }
        ruyi_ins_codes_add(context->codes, Ruyi_ir_Load, index);
        return NULL;
    }
    // TODO load from member
    
    // load from global
    if (ruyi_symtab_get_global_var_by_name(context->symtab, name, &var)) {
        index = var.index;
        if (out_type) {
            *out_type = var.type;
        }
        ruyi_ins_codes_add(context->codes, Ruyi_ir_Getglb, index);
        return NULL;
    }
    ruyi_unicode_string_encode_utf8_n(name, name_buf, NAME_BUF_LENGTH-1);
    return ruyi_error_misc("can not find variable %s", name_buf);
}

static ruyi_error* gen_integer(ruyi_cg_body_context *context, UINT64 value, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    UINT32 index;
    if (expect_type != NULL) {
        switch (expect_type->ir_type) {
            case Ruyi_ir_type_Byte:
            case Ruyi_ir_type_Rune:
            case Ruyi_ir_type_Int16:
            case Ruyi_ir_type_Int32:
            case Ruyi_ir_type_Int64:
                {
                    switch (value) {
                        case 0:
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Iconst_0, 0);
                            break;
                        case 1:
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Iconst_1, 0);
                            break;
                        case -1:
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Iconst_m1, 0);
                            break;
                        default:
                            index = ruyi_symtab_constants_pool_get_or_add_int64(context->symtab->cp, value);
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Iconst, index);
                            break;
                    }
                    if (out_type != NULL) {
                        out_type->ir_type = Ruyi_ir_type_Int64;
                        out_type->size = 8;
                        out_type->detail.uniptr = NULL;
                    }
                }
                break;
            case Ruyi_ir_type_Float32:
            case Ruyi_ir_type_Float64:
                {
                    switch (value) {
                        case 0:
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Fconst_0, 0);
                            break;
                        case 1:
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Fconst_1, 0);
                            break;
                        case -1:
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Fconst_m1, 0);
                            break;
                        default:
                            index = ruyi_symtab_constants_pool_get_or_add_float64(context->symtab->cp, (FLOAT64)value);
                            ruyi_ins_codes_add(context->codes, Ruyi_ir_Fconst, index);
                            break;
                    }
                    if (out_type != NULL) {
                        out_type->ir_type = Ruyi_ir_type_Float64;
                        out_type->size = 8;
                        out_type->detail.uniptr = NULL;
                    }
                }
                break;
            default:
                return ruyi_error_misc("unsupport ");
        }
        
    } else {
        index = ruyi_symtab_constants_pool_get_or_add_int64(context->symtab->cp, value);
        ruyi_ins_codes_add(context->codes, Ruyi_ir_Iconst, index);
        if (out_type != NULL) {
            out_type->ir_type = Ruyi_ir_type_Int64;
            out_type->size = 8;
            out_type->detail.uniptr = NULL;
        }
    }
    return NULL;
}

static
ruyi_error* gen_var_declaration(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error* err;
    const ruyi_unicode_string *name = (const ruyi_unicode_string *)ast_stmt->data.ptr_value;
    ruyi_ast *ast_type = ruyi_ast_get_child(ast_stmt, 0);
    ruyi_ast *ast_expr = ruyi_ast_get_child(ast_stmt, 1);
    ruyi_symtab_variable var;
    ruyi_symtab_type expr_type;
    ruyi_symtab_variable* var_ptr;
    UINT32 index;
    var.name = name;
    if (ast_type->type == Ruyi_at_var_declaration_auto_type) {
        // get type from expr
        // auto type, later fill by expr...
        var.type.ir_type = 0;
        var.type.size = 0;
        var.type.detail.uniptr = NULL;
    } else {
        handle_type(ast_type, &var.type);
    }
    if ((err = ruyi_symtab_function_scope_add_var(context->func->func_symtab_scope, &var, &index)) != NULL) {
        return err;
    }
    if (ast_expr == NULL) {
        if (ast_type->type == Ruyi_at_var_declaration_auto_type) {
            // must not be here
            assert(0);
        }
        // just define the variable, there was not init expression.
        return NULL;
    }
    if (ast_type->type == Ruyi_at_var_declaration_auto_type) {
        if ((err = gen_stmt(context, ast_expr, &expr_type, NULL)) != NULL) {
            return err;
        }
        // fill type back for auto_type
        var_ptr = ruyi_symtab_function_scope_get_var(context->func->func_symtab_scope, index);
        assert(var_ptr);
        var_ptr->type = expr_type;
    } else {
        if ((err = gen_stmt(context, ast_expr, &expr_type, &var.type)) != NULL) {
            return err;
        }
        if (!type_can_assign(&expr_type, &var.type)) {
            return make_unicode_name_error("var can not be assigned by diference type when define global var: %s", var.name);
        }
    }
    ruyi_ins_codes_add(context->codes, Ruyi_ir_Store, index);
    return NULL;
}

static ruyi_error* gen_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    switch (ast_stmt->type) {
        case Ruyi_at_return_statement:
            return gen_return_stmt(context, ast_stmt, out_type);
        case Ruyi_at_additive_expression:
            return gen_additive_expression(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_multiplicative_expression:
            return gen_multiplicative_expression(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_name:
            // load from variable name
            return gen_load_from_variable_name(context, (const ruyi_unicode_string *)ast_stmt->data.ptr_value, out_type, expect_type);
        case Ruyi_at_integer:
            return gen_integer(context, ast_stmt->data.int32_value, out_type, expect_type);
        case Ruyi_at_var_declaration:
            return gen_var_declaration(context, ast_stmt, out_type, expect_type);
        default:
            break;
    }
    return NULL;
}

static ruyi_error* gen_func_body(ruyi_cg_body_context *context, ruyi_ast *ast_body) {
    ruyi_error* err;
    UINT32 len, i;
    ruyi_ast *ast_stmt;
    assert(ast_body);
    assert(Ruyi_at_block_statements == ast_body->type);
    len = ruyi_ast_child_length(ast_body);
    for (i = 0; i < len; i++) {
        ast_stmt = ruyi_ast_get_child(ast_body, i);
        if ((err = gen_stmt(context, ast_stmt, NULL, NULL)) != NULL) {
            goto gen_func_body_error;
        }
    }
    return NULL;
gen_func_body_error:
    return err;
}

static ruyi_error* gen_global_func_define(ruyi_symtab *symtab, const ruyi_ast *ast, ruyi_vector *global_functions) {
    ruyi_error *err;
    ruyi_ast *ast_name = NULL;
    ruyi_ast *ast_type = NULL;
    ruyi_ast *ast_formal_params;
    ruyi_ast *ast_return_type;
    ruyi_ast *ast_body;
    ruyi_ast *temp;
    ruyi_symtab_type the_type;
    ruyi_symtab_function_define *func = NULL;
    ruyi_ins_codes *ins_codes = NULL;
    UINT32 i, len;
    ruyi_symtab_variable var;
    assert(ast);
    if (Ruyi_at_function_declaration == ast->type) {
        ast_name = ruyi_ast_get_child(ast, 0);
        ast_formal_params = ruyi_ast_get_child(ast, 1);
        ast_return_type = ruyi_ast_get_child(ast, 2);
        ast_body = ruyi_ast_get_child(ast, 3);
    } else if (Ruyi_at_anonymous_function_declaration == ast->type) {
        ast_formal_params = ruyi_ast_get_child(ast, 0);
        ast_return_type = ruyi_ast_get_child(ast, 1);
        ast_body = ruyi_ast_get_child(ast, 2);
    } else {
        err = ruyi_error_misc("the ast is not a function define.");
        goto gen_global_func_define_on_error;
    }
    if (ruyi_ast_child_length(ast_formal_params) > RUYI_MAX_UINT16) {
        err = ruyi_error_misc("too many function formal params count.");
        goto gen_global_func_define_on_error;
    }
    if (ast_name) {
        func = ruyi_symtab_function_create(symtab, (ruyi_unicode_string*)ast_name->data.ptr_value);
    } else {
        func = ruyi_symtab_function_create(symtab, NULL);
    }
    // return types
    if (ast_return_type == NULL) {
        // leave return type is null
    } else {
        len = ruyi_ast_child_length(ast_return_type);
        if (len > RUYI_MAX_UINT16) {
            err = ruyi_error_misc("too many function return values.");
            goto gen_global_func_define_on_error;
        }
        for (i = 0; i < len; i++) {
            temp = ruyi_ast_get_child(ast_return_type, i);
            if ((err = handle_type(temp, &the_type)) != NULL) {
                goto gen_global_func_define_on_error;
            }
            ruyi_symtab_function_add_return_type(func, the_type);
        }
    }
    // arguments
    len = ruyi_ast_child_length(ast_formal_params);
    if (len > RUYI_MAX_UINT16) {
        err = ruyi_error_misc("too many function formal parameters.");
        goto gen_global_func_define_on_error;
    }
    for (i = 0; i < len; i++) {
        temp = ruyi_ast_get_child(ast_formal_params, i);
        ast_name = ruyi_ast_get_child(temp, 0);
        ast_type = ruyi_ast_get_child(temp, 1);
        if ((err = handle_type(ast_type, &the_type)) != NULL) {
            goto gen_global_func_define_on_error;
        }
        var.name = (ruyi_unicode_string*)ast_name->data.ptr_value;
        var.type = the_type;
        ruyi_symtab_function_add_arg(func, &var);
    }
    // func body
    ins_codes = ruyi_ins_codes_create();
    ruyi_cg_body_context context;
    context.codes = ins_codes;
    context.func = func;
    context.symtab = symtab;
    if ((err = gen_func_body(&context, ast_body)) != NULL) {
        goto gen_global_func_define_on_error;
    }
    
    func->codes_size = ins_codes->len;
    if (ins_codes->len > 0) {
        func->codes = (UINT64 *) ruyi_mem_alloc(sizeof(UINT64) * ins_codes->len);
        memcpy(func->codes, ins_codes->data, sizeof(UINT64) * ins_codes->len);
    } else {
        func->codes = NULL;
    }
    
    ruyi_vector_add(global_functions, ruyi_value_ptr(func_create(func)));
    
    // TODO free func ???
    return NULL;
gen_global_func_define_on_error:
    if (func) {
        ruyi_symtab_function_destroy(func);
    }
    if (ins_codes) {
        ruyi_ins_codes_destroy(ins_codes);
    }
    return err;
}

static BYTE* create_bytes_data_from_unicode(const ruyi_unicode_string *str, UINT32 *out_len) {
    ruyi_bytes_string* bytes_string;
    char *bytes;
    if (!str) {
        return NULL;
    }
    bytes_string = ruyi_unicode_string_encode_utf8(str);
    bytes = (BYTE *)ruyi_mem_alloc(bytes_string->length + 1);
    memcmp(bytes, bytes_string->str, bytes_string->length);
    ruyi_unicode_bytes_string_destroy(bytes_string);
    bytes[bytes_string->length] = '\0';
    if (out_len) {
        *out_len = bytes_string->length + 1;
    }
    return bytes;
}

static ruyi_error* gen_global(ruyi_symtab *symtab, const ruyi_ast *ast, ruyi_cg_file *ir_file) {
    ruyi_error *err = NULL;
    UINT32 len, i;
    ruyi_ast *global_ast;
    ruyi_vector *global_vars = NULL;    // the item's type is 'ruyi_cg_file_global_var'
    ruyi_vector *global_functions = NULL;
    ruyi_vector *global_classes = NULL;
    ruyi_value temp_value;
    ruyi_symtab_constant *c;
    ruyi_cg_file_const_pool *cfcp;
    UINT32 c_len;
    if (!ast) {
        return NULL;
    }
    global_vars = ruyi_vector_create();
    global_functions = ruyi_vector_create();
    global_classes = ruyi_vector_create();
    len = ruyi_ast_child_length(ast);
    do {
        for (i = 0; i < len; i++) {
            global_ast = ruyi_ast_get_child(ast, i);
            if (!global_ast) {
                err = ruyi_error_misc("global define can not be empty");
                goto gen_global_on_error;
            }
            switch (global_ast->type) {
                case Ruyi_at_var_declaration:
                    if ((err = gen_global_var_define(symtab, global_ast, global_vars)) != NULL) {
                        goto gen_global_on_error;
                    }
                    break;
                case Ruyi_at_function_declaration:
                    if ((err = gen_global_func_define(symtab, global_ast, global_functions)) != NULL) {
                        goto gen_global_on_error;
                    }
                    break;
                // TODO class, constant etc ...
                default:
                    break;
            }
        }
    } while(0);
    // global vars
    ir_file->gv_count = ruyi_vector_length(global_vars);
    if (ir_file->gv_count > 0) {
        ir_file->gv = (ruyi_cg_file_global_var**)ruyi_mem_alloc(ir_file->gv_count * sizeof(ruyi_cg_file_global_var*));
        len = ruyi_vector_length(global_vars);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(global_vars, i, &temp_value);
            ir_file->gv[i] = (ruyi_cg_file_global_var*)temp_value.data.ptr;
        }
    } else {
        ir_file->gv = NULL;
    }
    if (global_vars) {
        // item in this vector will use in ir_file->gv, so it just only free vector itself.
        ruyi_vector_destroy(global_vars);
    }
    
    // functions
    ir_file->func_count = ruyi_vector_length(global_functions);
    if (ir_file->func_count > 0) {
        ir_file->func = (ruyi_cg_file_function**)ruyi_mem_alloc(ir_file->func_count * sizeof(ruyi_cg_file_function*));
        len = ruyi_vector_length(global_functions);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(global_functions, i, &temp_value);
            ir_file->func[i] = (ruyi_cg_file_function*)temp_value.data.ptr;
        }
    } else {
        ir_file->func = NULL;
    }
    if (global_functions) {
        // item in this vector will use in ir_file->func, so it just only free vector itself.
        ruyi_vector_destroy(global_functions);
    }
    
    // constant pool
    if (symtab->cp && symtab->cp->index2value) {
        len = ruyi_vector_length(symtab->cp->index2value);
        ir_file->cp_count = len;
        ir_file->cp = (ruyi_cg_file_const_pool**)ruyi_mem_alloc(sizeof(ruyi_cg_file_const_pool*) * len);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(symtab->cp->index2value, i, &temp_value);
            c = (ruyi_symtab_constant *)temp_value.data.ptr;
            cfcp = (ruyi_cg_file_const_pool*) ruyi_mem_alloc(sizeof(ruyi_cg_file_const_pool));
            cfcp->index = i;
            cfcp->type = c->type;
            
            ir_file->cp[i] = cfcp;
            switch (c->type) {
                case Ruyi_ir_type_Int64:
                    cfcp->value_size = 8;
                    cfcp->value.int64_value = c->data.int64_value;
                    break;
                case Ruyi_ir_type_Float64:
                    cfcp->value_size = 8;
                    cfcp->value.float64_value = c->data.float64_value;
                    break;
                case Ruyi_ir_type_String:
                    cfcp->value.str_value = create_bytes_data_from_unicode((const ruyi_unicode_string*)c->data.uncode_str, &c_len);
                    cfcp->value_size = c_len;
                    break;
                default:
                    err = ruyi_error_misc("unsupport constant pool type");
                    goto gen_global_on_error;
            }
        }
    } else {
        ir_file->cp_count = 0;
        ir_file->cp = NULL;
    }
    
    // TODO fill back to ir_file
   
    return NULL;
gen_global_on_error:
    if (global_vars) {
        RUYI_VECTOR_DESTROY_WITH_PTR_ITEMS(global_vars, ruyi_mem_free);
    }
    // TODO destroy vars, funcs, classes, constants etc...
    if (global_functions) {
        ruyi_vector_destroy(global_functions);
        // TODO
        
    }
    if (global_classes) {
        ruyi_vector_destroy(global_classes);
        
        // TODO
    }
    return err;
}

ruyi_error* ruyi_cg_generate(const ruyi_ast *ast, ruyi_cg_file **out_ir_file) {
    ruyi_error *err;
    ruyi_cg_file *file = NULL;
    ruyi_unicode_string *name;
    ruyi_symtab *symtab = NULL;
    ruyi_ast *ast_package, *ast_import, *ast_global;
    assert(ast);
    // TODO here use 'test1' name for test !
    name = ruyi_unicode_string_init_from_utf8("test1", 0);
    file = create_ruyi_cg_file(name);
    ruyi_unicode_string_destroy(name);
    if (Ruyi_at_root != ast->type) {
        err = ruyi_error_misc("need root ast");
        goto ruyi_cg_generate_on_error;
    }
    symtab = ruyi_symtab_create();
    ast_package = ruyi_ast_get_child(ast, 0);
    ast_import = ruyi_ast_get_child(ast, 1);
    ast_global = ruyi_ast_get_child(ast, 2);
    if ((err = gen_package(symtab, ast_package, file)) != NULL) {
        goto ruyi_cg_generate_on_error;
    }
    if ((err = gen_import(symtab, ast_import, file)) != NULL) {
        goto ruyi_cg_generate_on_error;
    }
    if ((err = gen_global(symtab, ast_global, file)) != NULL) {
        goto ruyi_cg_generate_on_error;
    }
    *out_ir_file = file;
    return NULL;
ruyi_cg_generate_on_error:
    if (file) {
        ruyi_cg_file_destroy(file);
    }
    if (symtab) {
        ruyi_symtab_destroy(symtab);
    }
    *out_ir_file = NULL;
    return err;
}
