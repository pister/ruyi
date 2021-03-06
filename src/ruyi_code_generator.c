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
#include "ruyi_unicode.h"
#include <string.h> // for memcpy

#define CG_FUNC_WRITE_CAP_INIT 16
#define CG_FILE_MAGIC_CODE 0xBBFFAA0098DAC699
#define CG_FILE_VERSION    0x01

#define PACKAGE_SEPARATE '.'

static ruyi_error* handle_type(ruyi_ast *ast_type, ruyi_symtab_type *out_type);

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

static void function_writer_append_code(ruyi_cg_function_writer *writer, UINT32 code) {
    UINT32 new_capacity;
    UINT32 *new_codes;
    UINT32 *old_codes;
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
    UINT32 code;
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
    function_writer->codes = (UINT32*)ruyi_mem_alloc(CG_FUNC_WRITE_CAP_INIT * sizeof(UINT32));
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
        func->codes = (UINT32 *)ruyi_mem_alloc(sizeof(UINT32) * func->codes_size);
        memcpy(func->codes, symtab_func->codes, sizeof(UINT32) * func->codes_size);
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
            err = ruyi_error_misc_unicode_name("miss initialize expression for auto-type when define global var: %s", var.name);
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
                err = ruyi_error_misc_unicode_name("var can not be assigned by diference type when define global var: %s", var.name);
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

static UINT32 ir_type_size(ruyi_ir_type type) {
    switch (type) {
        case Ruyi_ir_type_Void:
            return 1;
        case Ruyi_ir_type_Int8:
            return 1;
        case Ruyi_ir_type_Int16:
            return 2;
        case Ruyi_ir_type_Rune:
            return 4;
        case Ruyi_ir_type_Int32:
            return 4;
        case Ruyi_ir_type_Int64:
            return 8;
        case Ruyi_ir_type_Float32:
            return 4;
        case Ruyi_ir_type_Float64:
            return 8;
        case Ruyi_ir_type_Object:
            return 8;
        case Ruyi_ir_type_String:
            return 8;
        case Ruyi_ir_type_Array:
            return 8;
        case Ruyi_ir_type_Type:
            return 8;
        case Ruyi_ir_type_Map:
            return 8;
        case Ruyi_ir_type_Function:
            return 8;
        default:
            break;
    }
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
            out_type->ir_type = Ruyi_ir_type_Int8;
            break;
        case Ruyi_at_type_bool:
            out_type->ir_type = Ruyi_ir_type_Int8;
            break;
        case Ruyi_at_type_short:
            out_type->ir_type = Ruyi_ir_type_Int16;
            break;
        case Ruyi_at_integer:
        case Ruyi_at_type_int:
            out_type->ir_type = Ruyi_ir_type_Int32;
            break;
        case Ruyi_at_type_rune:
            out_type->ir_type = Ruyi_ir_type_Rune;
            break;
        case Ruyi_at_type_long:
            out_type->ir_type = Ruyi_ir_type_Int64;
            break;
        case Ruyi_at_type_float:
            out_type->ir_type = Ruyi_ir_type_Float32;
            break;
        case Ruyi_at_float:
        case Ruyi_at_type_double:
            out_type->ir_type = Ruyi_ir_type_Float64;
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
    out_type->size = ir_type_size(out_type->ir_type);
    return NULL;
}

typedef struct {
    UINT32 *data;
    UINT32 len;
    UINT32 cap;
} ruyi_ins_codes;

typedef struct {
    ruyi_symtab_function_define *func;
    ruyi_ins_codes              *codes;
    ruyi_symtab                 *symtab;    // reference of global ruyi_symtab
    ruyi_list                   *break_index_stack; // the item value is index-vector
    ruyi_list                   *continue_index_stack; // the item value is index-vector
} ruyi_cg_body_context;

static
ruyi_ins_codes* ruyi_ins_codes_create() {
    const UINT32 init_cap = 32;
    ruyi_ins_codes *codes = (ruyi_ins_codes*)ruyi_mem_alloc(sizeof(UINT32) * init_cap);
    codes->cap = init_cap;
    codes->len = 0;
    codes->data = (UINT32*)ruyi_mem_alloc(sizeof(UINT32) * codes->cap);
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

static ruyi_cg_body_context * ruyi_cg_body_context_create(ruyi_symtab *symtab, ruyi_symtab_function_define *func) {
    ruyi_cg_body_context * context = (ruyi_cg_body_context*) ruyi_mem_alloc(sizeof(ruyi_cg_body_context));
    context->symtab = symtab;
    context->func = func;
    context->codes = ruyi_ins_codes_create();
    context->break_index_stack = ruyi_list_create();
    context->continue_index_stack = ruyi_list_create();
    return context;
}

static void ruyi_cg_body_context_destroy(ruyi_cg_body_context *context) {
    if (!context) {
        return;
    }
    ruyi_ins_codes_destroy(context->codes);
    if (context->break_index_stack) {
        ruyi_list_destroy(context->break_index_stack);
    }
    if (context->continue_index_stack) {
        ruyi_list_destroy(context->continue_index_stack);
    }
}

static void ruyi_cg_body_context_push_break_continue(ruyi_cg_body_context *context) {
    ruyi_list_add_last(context->break_index_stack, ruyi_value_ptr(NULL));   // lazy init
    ruyi_list_add_last(context->continue_index_stack, ruyi_value_ptr(NULL)); // lazy init
}


static void ruyi_cg_body_context_pop_break_continue(ruyi_cg_body_context *context) {
    ruyi_value value;
    ruyi_vector *vector;
    if (!ruyi_list_empty(context->break_index_stack)) {
        ruyi_list_remove_last(context->break_index_stack, &value);
        vector = (ruyi_vector*)value.data.ptr;
        if (vector) {
            ruyi_vector_destroy(vector);
        }
    }
    if (!ruyi_list_empty(context->continue_index_stack)) {
        ruyi_list_remove_last(context->continue_index_stack, &value);
        vector = (ruyi_vector*)value.data.ptr;
        if (vector) {
            ruyi_vector_destroy(vector);
        }
    }
}

static void ruyi_cg_body_context_add_index(ruyi_list *stack, UINT32 index) {
    ruyi_vector *vector;
    ruyi_list_item *last;
    assert(stack);
    last = stack->last;
    assert(last);
    vector = (ruyi_vector*)last->value.data.ptr;
    if (vector == NULL) {
        vector = ruyi_vector_create();
        last->value = ruyi_value_ptr(vector);
    }
    ruyi_vector_add(vector, ruyi_value_uint32(index));
}


static
UINT32 ruyi_ins_codes_add(ruyi_ins_codes *codes, ruyi_ir_ins ins, UINT32 val) {
    UINT32 pos;
    while (codes->len >= codes->cap) {
        UINT32 new_cap = (UINT32)(codes->cap * 1.5 + 1);
        UINT32 *new_data = (UINT32*)ruyi_mem_alloc(sizeof(UINT32) * new_cap);
        memcpy(new_data, codes->data, sizeof(UINT32) * codes->cap);
        ruyi_mem_free(codes->data);
        codes->data = new_data;
        codes->cap = new_cap;
    }
    pos = codes->len;
    codes->data[codes->len++] = ruyi_ir_make_code(ins, val);
    return pos;
}


static
void ruyi_ins_codes_set_value(ruyi_ins_codes *codes, UINT32 index, UINT32 val) {
    UINT32 code = codes->data[index];
    ruyi_ir_ins ins;
    ruyi_ir_parse_code(code, &ins, NULL);
    codes->data[index] = ruyi_ir_make_code(ins, val);
}

static
ruyi_error* gen_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type);

static
ruyi_error* gen_block_statements(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type);

static void proccess_loop_begin(ruyi_cg_body_context *context);

static void proccess_loop_end(ruyi_cg_body_context *context, UINT32 index_for_loop_start);

static
ruyi_error* gen_return_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type) {
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

static
ruyi_error* gen_binary_expr_with_cast(const ruyi_symtab_type *left_type, const ruyi_symtab_type *right_type, ruyi_symtab_type *out_type, const ruyi_ast *op,
                   ruyi_ins_codes *codes, const ruyi_ast_type* ops, const ruyi_ir_ins *int64_ins, const ruyi_ir_ins *double_ins, UINT32 len) {
    int i;
    switch (left_type->ir_type) {
        case Ruyi_ir_type_Int8:
        case Ruyi_ir_type_Int16:
        case Ruyi_ir_type_Int32:
        case Ruyi_ir_type_Rune:
        case Ruyi_ir_type_Int64:
            switch (right_type->ir_type) {
                case Ruyi_ir_type_Int8:
                case Ruyi_ir_type_Int16:
                case Ruyi_ir_type_Int32:
                case Ruyi_ir_type_Rune:
                case Ruyi_ir_type_Int64:
                    for (i = 0; i < len; i++) {
                        if (ops[i] == op->type) {
                            ruyi_ins_codes_add(codes, int64_ins[i], 0);
                            break;
                        }
                    }
                    if (out_type) {
                        out_type->ir_type = left_type->ir_type;
                        out_type->detail.uniptr = NULL;
                    }
                    break;
                case Ruyi_ir_type_Float32:
                case Ruyi_ir_type_Float64:
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
                    }
                    break;
                default:
                    return ruyi_error_misc("unsupport cast type to int64");
            }
            break;
        case Ruyi_ir_type_Float32:
        case Ruyi_ir_type_Float64:
            switch (right_type->ir_type) {
                case Ruyi_ir_type_Int8:
                case Ruyi_ir_type_Int16:
                case Ruyi_ir_type_Int32:
                case Ruyi_ir_type_Rune:
                case Ruyi_ir_type_Int64:
                    ruyi_ins_codes_add(codes, Ruyi_ir_I2f, 0);
                    break;
                case Ruyi_ir_type_Float32:
                case Ruyi_ir_type_Float64:
                    break;
                default:
                    return ruyi_error_misc("unsupport cast type to float64");
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
            }
            break;
        default:
            return ruyi_error_misc("unknown left type: %d", left_type);
    }
    if (out_type) {
        out_type->size = ir_type_size(out_type->ir_type);
    }
    return NULL;
}

static
ruyi_error* gen_additive_expression(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err;
    UINT32 len = ruyi_ast_child_length(ast_stmt);
    ruyi_ast *left;
    ruyi_ast *right;
    ruyi_ast *op;
    ruyi_symtab_type left_type, right_type;
    const ruyi_ast_type ops[] = {Ruyi_at_op_add, Ruyi_at_op_sub};
    const ruyi_ir_ins int64_ins[] = {Ruyi_ir_Iadd, Ruyi_ir_Isub};
    const ruyi_ir_ins double_ins[] = {Ruyi_ir_Fadd, Ruyi_ir_Fsub};
    assert(3 == len);
    left = ruyi_ast_get_child(ast_stmt, 0);
    op = ruyi_ast_get_child(ast_stmt, 1);
    right = ruyi_ast_get_child(ast_stmt, 2);
    if ((err = gen_stmt(context, left, &left_type, expect_type)) != NULL) {
        return err;
    }
    if ((err = gen_stmt(context, right, &right_type, &left_type)) != NULL) {
        return err;
    }
    return gen_binary_expr_with_cast(&left_type, &right_type, out_type, op, context->codes, ops, int64_ins, double_ins, sizeof(ops)/sizeof(ops[0]));
}

static
ruyi_error* gen_multiplicative_expression(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err;
    UINT32 len = ruyi_ast_child_length(ast_stmt);
    ruyi_ast *left;
    ruyi_ast *right;
    ruyi_ast *op;
    ruyi_symtab_type left_type, right_type;
    const ruyi_ast_type ops[] = {Ruyi_at_op_mul, Ruyi_at_op_div, Ruyi_at_op_mod};
    const ruyi_ir_ins int64_ins[] = {Ruyi_ir_Imul, Ruyi_ir_Idiv, Ruyi_ir_Imod};
    const ruyi_ir_ins double_ins[] = {Ruyi_ir_Fadd, Ruyi_ir_Fsub, 0};
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

static
ruyi_error* gen_load_from_variable_name(ruyi_cg_body_context *context, const ruyi_unicode_string *name, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    UINT32 index;
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
    return ruyi_error_misc_unicode_name("can not find variable %s", name);
}

static
ruyi_error* gen_integer(ruyi_cg_body_context *context, UINT64 value, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    UINT32 index;
    if (expect_type != NULL) {
        switch (expect_type->ir_type) {
            case Ruyi_ir_type_Int8:
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
                        out_type->ir_type = expect_type->ir_type;
                        out_type->size = expect_type->size;
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
                return ruyi_error_misc("unsupport ir_type: %d", expect_type->ir_type);
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
    var.scope_type = Ruyi_sst_Local;
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
            return ruyi_error_misc_unicode_name("var can not be assigned by diference type when define global var: %s", var.name);
        }
    }
    ruyi_ins_codes_add(context->codes, Ruyi_ir_Store, index);
    return NULL;
}

static
ruyi_error* gen_while_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error* err;
    ruyi_ast *ast_expr = ruyi_ast_get_child(ast_stmt, 0);
    ruyi_ast *ast_body = ruyi_ast_get_child(ast_stmt, 1);
    ruyi_symtab_type expr_type;
    UINT32 index_for_loop_start = context->codes->len;
    UINT32 which_index_will_jump_out;
    
    proccess_loop_begin(context);
    
    // while enter
    if ((err = gen_stmt(context, ast_expr, &expr_type, NULL)) != NULL) {
        return err;
    }
    which_index_will_jump_out = ruyi_ins_codes_add(context->codes, Ruyi_ir_Jfalse, 0); // will fill later
    // body
    if ((err = gen_block_statements(context, ast_body, NULL, NULL)) != NULL) {
        return err;
    }
    // jump to while enter
    ruyi_ins_codes_add(context->codes, Ruyi_ir_Jmp, index_for_loop_start);
    ruyi_ins_codes_set_value(context->codes, which_index_will_jump_out, context->codes->len);
    
    proccess_loop_end(context, index_for_loop_start);
    
    return NULL;
}

static
BOOL is_last_ins_matches(const ruyi_cg_body_context *context, UINT32 ins) {
    UINT32 len = context->codes->len;
    UINT32 last_code;
    ruyi_ir_ins last_ins;
    if (len == 0) {
        return FALSE;
    }
    last_code = context->codes->data[len-1];
    ruyi_ir_parse_code(last_code, &last_ins, NULL);
    return ins == last_ins;
}

static BOOL can_be_add_jmp(const ruyi_cg_body_context *context) {
    static const ruyi_ir_ins must_not_add_jump_after_ins[] = {Ruyi_ir_Ret, Ruyi_ir_Jmp};
    UINT32 i;
    for (i = 0; i < sizeof(must_not_add_jump_after_ins) / sizeof(must_not_add_jump_after_ins[0]); i++) {
        if (is_last_ins_matches(context, must_not_add_jump_after_ins[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

static
ruyi_error* gen_if_expr_and_body(ruyi_cg_body_context *context, ruyi_ast *ast_expr, ruyi_ast *ast_body, ruyi_vector *end_of_stmt_placeholders) {
    ruyi_error *err;
    ruyi_symtab_type expr_type;
    UINT32 end_of_body_placeholder;
    UINT32 end_of_stmt_placeholder;
    // if-expr
    if ((err = gen_stmt(context, ast_expr, &expr_type, NULL)) != NULL) {
        return err;
    }
    
    end_of_body_placeholder = ruyi_ins_codes_add(context->codes, Ruyi_ir_Jfalse, 0); // will jump to end of body
    // if-body
    if ((err = gen_block_statements(context, ast_body, NULL, NULL)) != NULL) {
        return err;
    }
    // jump to endof if-stmt
    // if the body's last ins code is 'ret', must be not add 'jmp'
    if (can_be_add_jmp(context)) {
        end_of_stmt_placeholder = ruyi_ins_codes_add(context->codes, Ruyi_ir_Jmp, 0);  // will jump to end of the stmt
        ruyi_vector_add(end_of_stmt_placeholders, ruyi_value_uint32(end_of_stmt_placeholder));
    }
    ruyi_ins_codes_set_value(context->codes, end_of_body_placeholder, context->codes->len);
    return NULL;
}

static
ruyi_error* gen_if_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err = NULL;
    ruyi_ast *ast_expr = ruyi_ast_get_child(ast_stmt, 0);
    ruyi_ast *ast_body = ruyi_ast_get_child(ast_stmt, 1);
    ruyi_ast *elseif_stmt;
    ruyi_ast *tail_stmt;
    UINT32 i, len;
    ruyi_value value;
    ruyi_vector *end_of_stmt_placeholders;
    len = ruyi_ast_child_length(ast_stmt);
    assert(len >= 2);
    
    end_of_stmt_placeholders = ruyi_vector_create();
    
    if ((err = gen_if_expr_and_body(context, ast_expr, ast_body, end_of_stmt_placeholders)) != NULL) {
         goto gen_if_stmt_error;
    }
    for (i = 2; i < len - 1; i++) {
        // else-if stmt
        elseif_stmt = ruyi_ast_get_child(ast_stmt, i);
        assert(elseif_stmt->type == Ruyi_at_elseif_statement);
        ast_expr = ruyi_ast_get_child(elseif_stmt, 0);
        ast_body = ruyi_ast_get_child(elseif_stmt, 1);
        if ((err = gen_if_expr_and_body(context, ast_expr, ast_body, end_of_stmt_placeholders)) != NULL) {
            goto gen_if_stmt_error;
        }
    }
    // the last if-else-stmt or else-stmt
    if (len > 2) {
        tail_stmt = ruyi_ast_get_child(ast_stmt, len - 1);
        if (tail_stmt->type == Ruyi_at_elseif_statement) {
            ast_expr = ruyi_ast_get_child(tail_stmt, 0);
            ast_body = ruyi_ast_get_child(tail_stmt, 1);
            if ((err = gen_if_expr_and_body(context, ast_expr, ast_body, end_of_stmt_placeholders)) != NULL) {
                goto gen_if_stmt_error;
            }
        } else if (tail_stmt->type == Ruyi_at_else_statement) {
            ast_body = ruyi_ast_get_child(tail_stmt, 0);
            if ((err = gen_block_statements(context, ast_body, NULL, NULL)) != NULL) {
                goto gen_if_stmt_error;
            }
        } else {
            err = ruyi_error_misc("unknow if-else stmt type");
            goto gen_if_stmt_error;
        }
    }
    len = ruyi_vector_length(end_of_stmt_placeholders);
    for (i = 0; i < len; i++) {
        ruyi_vector_get(end_of_stmt_placeholders, i, &value);
        ruyi_ins_codes_set_value(context->codes, value.data.uint32_value, context->codes->len);
    }
    
gen_if_stmt_error:
    if (end_of_stmt_placeholders) {
        ruyi_vector_destroy(end_of_stmt_placeholders);
    }
    return err;
}

static
ruyi_error* gen_relational_expression(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err;
    UINT32 len = ruyi_ast_child_length(ast_stmt);
    ruyi_ast *left;
    ruyi_ast *right;
    ruyi_ast *op;
    ruyi_symtab_type left_type, right_type;
    const ruyi_ast_type ops[] = {Ruyi_at_op_lt, Ruyi_at_op_lte, Ruyi_at_op_gt, Ruyi_at_op_gte};
    const ruyi_ir_ins int64_ins[] = {Ruyi_ir_Icmp_lt, Ruyi_ir_Icmp_lte, Ruyi_ir_Icmp_gt, Ruyi_ir_Icmp_gte};
    const ruyi_ir_ins double_ins[] = {Ruyi_ir_Fcmp_lt, Ruyi_ir_Fcmp_lte, Ruyi_ir_Fcmp_gt, Ruyi_ir_Fcmp_gte};
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
    if (op->type == Ruyi_at_op_instanceof) {
        return ruyi_error_misc("unsupport operator 'instanceof' at this version!");
    } else{
        return gen_binary_expr_with_cast(&left_type, &right_type, out_type, op, context->codes, ops, int64_ins, double_ins, sizeof(ops)/sizeof(ops[0]));
    }
}

static
ruyi_error* gen_block_statements(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err;
    ruyi_ast *ast_child_stmt;
    UINT32 i, len;
    ruyi_symtab_function_scope_enter(context->func->func_symtab_scope);
    len = ruyi_ast_child_length(ast_stmt);
    for (i = 0; i < len; i++) {
        ast_child_stmt = ruyi_ast_get_child(ast_stmt, i);
        if ((err = gen_stmt(context, ast_child_stmt, out_type, expect_type)) != NULL) {
            return err;
        }
    }
    ruyi_symtab_function_scope_leave(context->func->func_symtab_scope);
    return NULL;
}

static
ruyi_error* gen_assign_statement(ruyi_cg_body_context *context, const ruyi_ast *left_ast, ruyi_ast *expr_ast, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error *err;
    const ruyi_unicode_string *name;
    ruyi_symtab_variable var;
    ruyi_symtab_type expr_type;
    if (Ruyi_at_name == left_ast->type) {
        name = (const ruyi_unicode_string *)left_ast->data.ptr_value;
        if (!ruyi_symtab_function_scope_get(context->func->func_symtab_scope, name, &var)) {
            return ruyi_error_misc_unicode_name("can not find variable %s", name);
        }
        if ((err = gen_stmt(context, expr_ast, &expr_type, &var.type)) != NULL) {
            return err;
        }
        if (expr_type.ir_type != var.type.ir_type) {
            // TODO type auto cast ...
            return ruyi_error_misc_unicode_name("assign type not match: %s ", name);
        }
        
        ruyi_ins_codes_add(context->codes, Ruyi_ir_Store, var.index);
    } else {
        switch (left_ast->type) {
            case Ruyi_at_field_dot_access_expression:
                // TODO
                return ruyi_error_misc("do not support now...");
                break;
            case Ruyi_at_field_bracket_access_expression:
                // TODO
                return ruyi_error_misc("do not support now...");
                break;
            case Ruyi_at_array_variable_access:
                // TODO
                return ruyi_error_misc("do not support now...");
                break;
            case Ruyi_at_array_primary_access:
                // TODO
                return ruyi_error_misc("do not support now...");
                break;
            default:
                break;
        }
    }
    return NULL;
}

static
ruyi_error* gen_inc_or_dec_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type, BOOL incr) {
    ruyi_symtab_variable var;
    const ruyi_unicode_string *name = (const ruyi_unicode_string *)ast_stmt->data.ptr_value;
    if (!ruyi_symtab_function_scope_get(context->func->func_symtab_scope, name, &var)) {
        return ruyi_error_misc_unicode_name("can not find variable %s", name);
    }
    switch (var.scope_type) {
        case Ruyi_sst_Local:
            ruyi_ins_codes_add(context->codes, Ruyi_ir_Load, var.index);
            break;
        case Ruyi_sst_Global:
            ruyi_ins_codes_add(context->codes, Ruyi_ir_Getglb, var.index);
            break;
        case Ruyi_sst_Member:
            // TODO
            break;
        default:
            return ruyi_error_misc_unicode_name("unknown variable %s's scope type", name);
    }
    switch (var.type.ir_type) {
        case Ruyi_ir_type_Int8:
        case Ruyi_ir_type_Int16:
        case Ruyi_ir_type_Rune:
        case Ruyi_ir_type_Int32:
        case Ruyi_ir_type_Int64:
            if (incr) {
                ruyi_ins_codes_add(context->codes, Ruyi_ir_Iinc, 0);
            } else {
                ruyi_ins_codes_add(context->codes, Ruyi_ir_Idec, 0);
            }
            break;
        case Ruyi_ir_type_Float32:
        case Ruyi_ir_type_Float64:
            if (incr) {
                ruyi_ins_codes_add(context->codes, Ruyi_ir_Finc, 0);
            } else {
                ruyi_ins_codes_add(context->codes, Ruyi_ir_Fdec, 0);
            }
            break;
        default:
            return ruyi_error_misc_unicode_name("unknown variable %s's type", name);
    }
    switch (var.scope_type) {
        case Ruyi_sst_Local:
            ruyi_ins_codes_add(context->codes, Ruyi_ir_Store, var.index);
            break;
        case Ruyi_sst_Global:
            ruyi_ins_codes_add(context->codes, Ruyi_ir_Setglb, var.index);
            break;
        case Ruyi_sst_Member:
            // TODO
            break;
        default:
            return ruyi_error_misc_unicode_name("unknown variable %s's scope type", name);
    }
    return NULL;
}

static
ruyi_error* gen_left_hand_side_expression(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_ast *left_ast = ruyi_ast_get_child(ast_stmt, 0);
    ruyi_ast *tail_ast = ruyi_ast_get_child(ast_stmt, 1);
    
    switch (tail_ast->type) {
        case Ruyi_at_assign_statement:
            return gen_assign_statement(context, left_ast, ruyi_ast_get_child(tail_ast, 0), out_type, expect_type);
        case Ruyi_at_var_declaration:
            return gen_var_declaration(context, tail_ast, out_type, expect_type);
        case Ruyi_at_inc_statement:
            return gen_inc_or_dec_stmt(context, left_ast, out_type, expect_type, TRUE);
        case Ruyi_at_dec_statement:
            return gen_inc_or_dec_stmt(context, left_ast, out_type, expect_type, FALSE);
        case Ruyi_at_function_invocation_statement:
            
            break;
        default:
            break;
    }
    return NULL;
}

#define NAME_BUF_LENGTH 128

static
ruyi_error* gen_function_invocation(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error* err;
    UINT32 i, len;
    const ruyi_unicode_string *name = (const ruyi_unicode_string *)ruyi_ast_get_child(ast_stmt, 0)->data.ptr_value;
    ruyi_ast *ast_func_invoce_tail = ruyi_ast_get_child(ast_stmt, 1);
    ruyi_ast *ast_arg_list = ruyi_ast_get_child(ast_func_invoce_tail, 0);
    ruyi_ast *ast_arg;
    ruyi_symtab_function func;
    ruyi_symtab_type arg_out;
    char temp_name[NAME_BUF_LENGTH];
    assert(ast_arg_list != NULL);
    assert(ast_arg_list->type == Ruyi_at_argument_list);
    
    if (!ruyi_symtab_get_function_by_name(context->symtab, name, &func)) {
        return ruyi_error_misc_unicode_name("can not found function: %s", name);
    }
    
    len = ruyi_ast_child_length(ast_arg_list);
    
    if (len != func.parameter_count) {
        return ruyi_error_misc_unicode_name("parameters length is not match when calling function: %s", name);
    }
    if (out_type != NULL) {
        if (func.return_count != 1) {
            return ruyi_error_misc_unicode_name("too many return values when calling function: %s", name);
        }
        *out_type =  func.return_types[0];
    }
    
    //  the args order is Left to Right
    for (i = 0; i < len; i++) {
        ast_arg = ruyi_ast_get_child(ast_arg_list, i);
        if ((err = gen_stmt(context, ast_arg, &arg_out, NULL)) != NULL) {
            return err;
        }
        if (arg_out.ir_type !=  func.parameter_types[i].ir_type) {
            ruyi_unicode_string_encode_utf8_n(name, temp_name, NAME_BUF_LENGTH-1);
            return ruyi_error_misc("the argument %d's type was not matched when invoke method: %s", i, temp_name);
        }
    }
    ruyi_ins_codes_add(context->codes, Ruyi_ir_Invokesp, func.index);
    return NULL;
}

static void proccess_loop_begin(ruyi_cg_body_context *context) {
    ruyi_cg_body_context_push_break_continue(context);
}

static void proccess_loop_end(ruyi_cg_body_context *context, UINT32 index_for_loop_start) {
    UINT32 i, len;
    ruyi_value value;
    // update break index
    ruyi_vector *vector;
    if (!ruyi_list_empty(context->break_index_stack)) {
        ruyi_list_get_last(context->break_index_stack, &value);
        vector = (ruyi_vector*)value.data.ptr;
        if (vector) {
            len = ruyi_vector_length(vector);
            for (i = 0; i < len; i++) {
                ruyi_vector_get(vector, i, &value);
                ruyi_ins_codes_set_value(context->codes, value.data.uint32_value, context->codes->len);
            }
        }
    }
    if (!ruyi_list_empty(context->continue_index_stack)) {
        ruyi_list_get_last(context->continue_index_stack, &value);
        vector = (ruyi_vector*)value.data.ptr;
        if (vector) {
            len = ruyi_vector_length(vector);
            for (i = 0; i < len; i++) {
                ruyi_vector_get(vector, i, &value);
                ruyi_ins_codes_set_value(context->codes, value.data.uint32_value, index_for_loop_start);
            }
        }
    }
    ruyi_cg_body_context_pop_break_continue(context);
}

static
ruyi_error* gen_for_3_parts_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error* err;
    ruyi_ast *ast_for_three_parts = ruyi_ast_get_child(ast_stmt, 0);
    ruyi_ast *ast_for_body;
    ruyi_ast *ast_for_init;
    ruyi_ast *ast_expression;
    ruyi_ast *ast_for_update;
    ruyi_ast *ast_temp;
    UINT32 i, len;
    UINT32 index_for_loop_start;
    assert(ast_for_three_parts->type == Ruyi_at_for_3_parts_header);
    ast_for_init = ruyi_ast_get_child(ast_for_three_parts, 0);
    ast_expression = ruyi_ast_get_child(ast_for_three_parts, 1);
    ast_for_update = ruyi_ast_get_child(ast_for_three_parts, 2);
    ast_for_body = ruyi_ast_get_child(ast_stmt, 1);
    
    assert(ast_for_init->type == Ruyi_at_expr_statement_list);
    
    proccess_loop_begin(context);
    
    // for init part variable define scope can be accessed by body
    ruyi_symtab_function_scope_enter(context->func->func_symtab_scope);
    // init for
    len = ruyi_ast_child_length(ast_for_init);
    for (i = 0; i < len; i++) {
        ast_temp = ruyi_ast_get_child(ast_for_init, i);
        if ((err = gen_stmt(context, ast_temp, NULL, NULL)) != NULL) {
            return err;
        }
    }
    index_for_loop_start = context->codes->len;
    // body
    if ((err = gen_stmt(context, ast_for_body, NULL, NULL)) != NULL) {
        return err;
    }
    // for update
    len = ruyi_ast_child_length(ast_for_update);
    for (i = 0; i < len; i++) {
        ast_temp = ruyi_ast_get_child(ast_for_update, i);
        if ((err = gen_stmt(context, ast_temp, NULL, NULL)) != NULL) {
            return err;
        }
    }
    // for condition expression
    if ((err = gen_stmt(context, ast_expression, NULL, NULL)) != NULL) {
        return err;
    }
    ruyi_ins_codes_add(context->codes, Ruyi_ir_Jtrue, index_for_loop_start);
    // end of for
    proccess_loop_end(context, index_for_loop_start);
    ruyi_symtab_function_scope_leave(context->func->func_symtab_scope);
    return NULL;
}

static
ruyi_error* gen_bool(ruyi_cg_body_context *context, BOOL value, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    if (value) {
        ruyi_ins_codes_add(context->codes, Ruyi_ir_Iconst_1, 0);
    } else {
        ruyi_ins_codes_add(context->codes, Ruyi_ir_Iconst_0, 0);
    }
    if (out_type) {
        out_type->ir_type = Ruyi_ir_type_Int32;
        out_type->size = 4;
    }
    return NULL;
}


static
ruyi_error* gen_break_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    UINT32 index;
    index = ruyi_ins_codes_add(context->codes, Ruyi_ir_Jmp, 0);
    ruyi_cg_body_context_add_index(context->break_index_stack, index);
    return NULL;
}

static
ruyi_error* gen_continue_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    UINT32 index;
    index = ruyi_ins_codes_add(context->codes, Ruyi_ir_Jmp, 0);
    ruyi_cg_body_context_add_index(context->continue_index_stack, index);
    return NULL;
}

static void push_integer(ruyi_cg_body_context *context, INT64 value) {
    UINT32 index;
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
            ruyi_ins_codes_add(context->codes, Ruyi_ir_Push, index);
            break;
    }
}

static
ruyi_error* gen_array_creation_with_init(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
    ruyi_error* err;
    ruyi_ast *array_type = ruyi_ast_get_child(ast_stmt, 0);
    ruyi_ast *ast_expr;
    UINT32 ast_child_len = ruyi_ast_child_length(ast_stmt);
    UINT32 i;
    // TODO -------
    ruyi_symtab_type the_type;
    ruyi_symtab_type array_item_type;
    UINT32 array_len = ast_child_len - 1;
    // TODO in handle() array_item_type may has mem_alloc, please free it when array_item_type destroyed !!!
    if ((err = handle_type(array_type, &array_item_type)) != NULL) {
        return err;
    }

    // cap
    push_integer(context, array_len);
    // length
    push_integer(context, array_len);

    // TODO
    // type
    
  //  ruyi_symtab_constants_pool_get_or_parse(context->symtab->cp, )
    
    // new array
    
    // init item-values
    for (i = 1; i < ast_child_len; i++) {
        ast_expr = ruyi_ast_get_child(ast_stmt, i);
        // item
        if ((err = gen_stmt(context, ast_expr, &the_type, &array_item_type)) != NULL) {
            return err;
        }
    }
    
    
    
    
    return NULL;
}

static
ruyi_error* gen_stmt(ruyi_cg_body_context *context, ruyi_ast *ast_stmt, ruyi_symtab_type *out_type, const ruyi_symtab_type *expect_type) {
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
        case Ruyi_at_while_statement:
            return gen_while_stmt(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_if_statement:
            return gen_if_stmt(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_relational_expression:
            return gen_relational_expression(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_block_statements:
            return gen_block_statements(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_left_hand_side_expression:
            return gen_left_hand_side_expression(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_function_invocation:
            return gen_function_invocation(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_for_3_parts_statement:
            return gen_for_3_parts_stmt(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_bool:
            return gen_bool(context, (BOOL)ast_stmt->data.int32_value, out_type, expect_type);
        case Ruyi_at_break_statement:
            return gen_break_stmt(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_continue_statement:
            return gen_continue_stmt(context, ast_stmt, out_type, expect_type);
        case Ruyi_at_array_creation_with_init:
            return gen_array_creation_with_init(context, ast_stmt, out_type, expect_type);
        default:
            break;
    }
    return NULL;
}

static
ruyi_error* gen_func_body(ruyi_cg_body_context *context, ruyi_ast *ast_body) {
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

static
ruyi_error* gen_global_func_define(ruyi_symtab *symtab, const ruyi_ast *ast, ruyi_vector *global_functions) {
    ruyi_error *err = NULL;
    ruyi_ast *ast_name = NULL;
    ruyi_ast *ast_type = NULL;
    ruyi_ast *ast_formal_params;
    ruyi_ast *ast_return_type;
    ruyi_ast *ast_body;
    ruyi_ast *temp;
    ruyi_symtab_type the_type;
    ruyi_symtab_function_define *func = NULL;
    UINT32 i, parameter_len, return_len;
    ruyi_symtab_variable var;
    ruyi_cg_body_context *context = NULL;
    ruyi_symtab_type paramter_types[RUYI_FUNC_MAX_PARAMETER_COUNT];
    ruyi_symtab_type return_types[RUYI_FUNC_MAX_RETURN_COUNT];

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
        if ((err = ruyi_symtab_function_create(symtab, (ruyi_unicode_string*)ast_name->data.ptr_value, &func)) != NULL) {
            goto gen_global_func_define_on_error;
        }
    } else {
        if ((err = ruyi_symtab_function_create(symtab, NULL, &func)) != NULL) {
            goto gen_global_func_define_on_error;
        }
    }
    // return types
    if (ast_return_type == NULL) {
        // leave return type is null
        return_len = 0;
    } else {
        return_len = ruyi_ast_child_length(ast_return_type);
        if (return_len > RUYI_FUNC_MAX_RETURN_COUNT) {
            err = ruyi_error_misc("too many function return values.");
            goto gen_global_func_define_on_error;
        }
        for (i = 0; i < return_len; i++) {
            temp = ruyi_ast_get_child(ast_return_type, i);
            if ((err = handle_type(temp, &the_type)) != NULL) {
                goto gen_global_func_define_on_error;
            }
            return_types[i] = the_type;
            ruyi_symtab_function_add_return_type(func, the_type);
        }
    }
    
    if (!ruyi_symtab_function_update_return_types(symtab, func->index, return_len, return_types)) {
        err = ruyi_error_misc("not match function by index: %d", func->index);
        goto gen_global_func_define_on_error;
    }
    
    // parameters
    parameter_len = ruyi_ast_child_length(ast_formal_params);
    if (parameter_len > RUYI_FUNC_MAX_PARAMETER_COUNT) {
        err = ruyi_error_misc("too many function formal parameters.");
        goto gen_global_func_define_on_error;
    }
    for (i = 0; i < parameter_len; i++) {
        temp = ruyi_ast_get_child(ast_formal_params, i);
        ast_name = ruyi_ast_get_child(temp, 0);
        ast_type = ruyi_ast_get_child(temp, 1);
        if ((err = handle_type(ast_type, &the_type)) != NULL) {
            goto gen_global_func_define_on_error;
        }
        var.name = (ruyi_unicode_string*)ast_name->data.ptr_value;
        var.type = the_type;
        paramter_types[i] = var.type;
        ruyi_symtab_function_add_arg(func, &var);
    }
    
    if (!ruyi_symtab_function_update_parameter_types(symtab, func->index, parameter_len, paramter_types)) {
        err = ruyi_error_misc("not match function by index: %d", func->index);
        goto gen_global_func_define_on_error;
    }
    // func body
    context = ruyi_cg_body_context_create(symtab, func);
    if ((err = gen_func_body(context, ast_body)) != NULL) {
        goto gen_global_func_define_on_error;
    }
    
    func->codes_size = context->codes->len;
    if (context->codes->len > 0) {
        func->codes = (UINT32 *) ruyi_mem_alloc(sizeof(UINT32) * context->codes->len);
        memcpy(func->codes, context->codes->data, sizeof(UINT32) *context->codes->len);
    } else {
        func->codes = NULL;
    }
    
    ruyi_vector_add(global_functions, ruyi_value_ptr(func_create(func)));
    
gen_global_func_define_on_error:
    if (func) {
        ruyi_symtab_function_destroy(func);
    }
    if (context) {
        ruyi_cg_body_context_destroy(context);
    }
    return err;
}

static
BYTE* create_bytes_data_from_unicode(const ruyi_unicode_string *str, UINT32 *out_len) {
    ruyi_bytes_string* bytes_string;
    BYTE *bytes;
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

static
ruyi_error* gen_global(ruyi_symtab *symtab, const ruyi_ast *ast, ruyi_cg_file *ir_file) {
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
    // TODO deal for bytes order
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
