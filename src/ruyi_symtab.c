//
//  ruyi_symtab.c
//  ruyi
//
//  Created by Songli Huang on 2019/10/31.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_symtab.h"
#include "ruyi_mem.h"
#include "ruyi_hashtable.h"
#include "ruyi_vector.h"
#include "ruyi_error.h"
#include <string.h> // for memset

#define NAME_BUF_LENGTH 128

static
ruyi_symtab_index_hashtable* index_hashtable_create(void) {
    ruyi_symtab_index_hashtable *table = (ruyi_symtab_index_hashtable*)ruyi_mem_alloc(sizeof(ruyi_symtab_index_hashtable));
    table->name2index = ruyi_hashtable_create();    // key: unicode, value: index
    table->index2var = ruyi_vector_create();        // type of item: ruyi_symtab_global_var
    return table;
}

static
void index_hashtable_destroy(ruyi_symtab_index_hashtable *table) {
    INT32 i;
    INT32 len;
    ruyi_value value;
    ruyi_symtab_global_var *var;
    if (!table) {
        return;
    }
    if (table->name2index) {
        ruyi_hashtable_destroy(table->name2index);
        // the unicode string will be free with vector destroy
    }
    if (table->index2var) {
        len = ruyi_vector_length(table->index2var);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(table->index2var, i, &value);
            var = (ruyi_symtab_global_var *)value.data.ptr;
            assert(var);
            ruyi_unicode_string_destroy((ruyi_unicode_string*)var->name);
            ruyi_mem_free(var);
        }
        ruyi_vector_destroy(table->index2var);
    }
    ruyi_mem_free(table);
}

static
ruyi_error* index_hashtable_add(ruyi_symtab_index_hashtable *table, const ruyi_symtab_global_var *var, UINT32 *index_out) {
    UINT32 index = 0;
    ruyi_value value;
    char name[NAME_BUF_LENGTH] = {0};
    ruyi_symtab_global_var *var_copied;
    if (ruyi_hashtable_get(table->name2index, ruyi_value_unicode_str(var->name), &value)) {
        ruyi_unicode_string_encode_utf8_n(var->name, name, NAME_BUF_LENGTH-1);
        return ruyi_error_syntax("duplicated var define: %s", name);
    }
    
    index = ruyi_vector_length(table->index2var);

    var_copied = ruyi_mem_alloc(sizeof(ruyi_symtab_global_var));
    var_copied->type = var->type;
    var_copied->var_size = var->var_size;
    var_copied->name = ruyi_unicode_string_copy_from(var->name);
    var_copied->index = index;
    
    ruyi_vector_add(table->index2var, ruyi_value_ptr(var_copied));
    ruyi_hashtable_put(table->name2index, ruyi_value_unicode_str(var_copied->name), ruyi_value_uint32(index));
    if (index_out) {
        *index_out = index;
    }
    return NULL;
}

static
BOOL index_hashtable_get_by_name(const ruyi_symtab_index_hashtable *table, const ruyi_unicode_string* name, ruyi_symtab_global_var *out_var) {
    ruyi_value index_value;
    ruyi_value item_value;
    const ruyi_symtab_global_var* var;
    assert(out_var);
    if (!ruyi_hashtable_get(table->name2index, ruyi_value_unicode_str((ruyi_unicode_string*)name), &index_value)) {
        return FALSE;
    }
    if (!ruyi_vector_get(table->index2var, index_value.data.uint32_value, &item_value)) {
        return FALSE;
    }
    var = (ruyi_symtab_global_var* )item_value.data.ptr;
    assert(var);
    out_var->index = var->index;
    out_var->type = var->type;
    out_var->var_size = var->var_size;
    // name will not be returned because caller knows the name.
    return TRUE;
}

ruyi_function_scope* ruyi_symtab_function_scope_create(void) {
    ruyi_function_scope *function_scope = (ruyi_function_scope *)ruyi_mem_alloc(sizeof(ruyi_function_scope));
    function_scope->block_scope_stack = ruyi_vector_create();
    return function_scope;
}

void ruyi_symtab_function_scope_destroy(ruyi_function_scope *function_scope) {
    if (NULL == function_scope) {
        return;
    }
    if (function_scope->block_scope_stack) {
        ruyi_vector_destroy(function_scope->block_scope_stack);
    }
    ruyi_mem_free(function_scope);
}

ruyi_symtab* ruyi_symtab_create(void) {
    ruyi_symtab *symtab = (ruyi_symtab *)ruyi_mem_alloc(sizeof(ruyi_symtab));
    symtab->global_variables = index_hashtable_create();
    symtab->function_scope = ruyi_symtab_function_scope_create();
    symtab->functions = ruyi_hashtable_create();
    symtab->constants = ruyi_hashtable_create();
    return symtab;
}

void ruyi_symtab_destroy(ruyi_symtab *symtab) {
    if (NULL == symtab) {
        return;
    }
    if (symtab->constants) {
        ruyi_hashtable_destroy(symtab->constants);
    }
    if (symtab->functions) {
        ruyi_hashtable_destroy(symtab->functions);
    }
    if (symtab->function_scope) {
        ruyi_symtab_function_scope_destroy(symtab->function_scope);
    }
    if (symtab->global_variables) {
        index_hashtable_destroy(symtab->global_variables);
    }
    ruyi_mem_free(symtab);
}

void ruyi_symtab_enter_scope(ruyi_symtab *symtab) {
    
}

void ruyi_symtab_leave_scope(ruyi_symtab *symtab) {
    
}

ruyi_error* ruyi_symtab_add_global_var(ruyi_symtab *symtab, const ruyi_symtab_global_var *var, UINT32 *out_index) {
    assert(symtab);
    return index_hashtable_add(symtab->global_variables, var, out_index);
}

ruyi_symtab_function_define* ruyi_symtab_function_create(const ruyi_unicode_string *name) {
    ruyi_symtab_function_define *func = (ruyi_symtab_function_define*)ruyi_mem_alloc(sizeof(ruyi_symtab_function_define));
    func->name = name;
    if (name) {
        func->anonymous = FALSE;
    } else {
        func->anonymous = TRUE;
    }
    func->return_types = NULL;
    func->args_types = NULL;
    func->codes = ruyi_vector_create();
    func->index = 0;
    return func;
}

void ruyi_symtab_function_destroy(ruyi_symtab_function_define* func) {
    UINT32 i, len;
    ruyi_value temp;
    ruyi_symtab_type type;
    ruyi_unicode_string *name;
    if (!func) {
        return;
    }
    if (func->return_types) {
        len = ruyi_vector_length(func->return_types);
        for (i = 0; i < len; i += 2) {
            ruyi_vector_get(func->return_types, i, &temp);
            type.ir_type = temp.data.int32_value;
            ruyi_vector_get(func->return_types, i+1, &temp);
            type.detail.uniptr = temp.data.ptr;
            ruyi_symtab_type_destroy(type);
        }
        ruyi_vector_destroy(func->return_types);
    }
    if (func->args_types) {
        len = ruyi_vector_length(func->args_types);
        for (i = 0; i < len; i += 3) {
            ruyi_vector_get(func->args_types, i, &temp);
            name = (ruyi_unicode_string *)temp.data.unicode_str;
            ruyi_unicode_string_destroy(name);

            ruyi_vector_get(func->args_types, i+1, &temp);
            type.ir_type = temp.data.int32_value;
            ruyi_vector_get(func->args_types, i+2, &temp);
            type.detail.uniptr = temp.data.ptr;
            ruyi_symtab_type_destroy(type);
        }
        ruyi_vector_destroy(func->args_types);
    }
    if (func->codes) {
        ruyi_vector_destroy(func->codes);
    }
    ruyi_mem_free(func);
}

void ruyi_symtab_function_add_arg(ruyi_symtab_function_define* func, const ruyi_unicode_string *name, ruyi_symtab_type type) {
    assert(func);
    if (func->args_types == NULL) {
        func->args_types = ruyi_vector_create();
    }
    // name
    ruyi_vector_add(func->args_types, ruyi_value_unicode_str(name));
    // ir_type
    ruyi_vector_add(func->args_types, ruyi_value_int32(type.ir_type));
    // detail
    ruyi_vector_add(func->args_types, ruyi_value_ptr(type.detail.uniptr));
    // TODO to create name ==> index mapping ?
}


void ruyi_symtab_function_add_return_type(ruyi_symtab_function_define* func, ruyi_symtab_type return_type) {
    assert(func);
    if (func->return_types == NULL) {
        func->return_types = ruyi_vector_create();
    }
    // ir_type
    ruyi_vector_add(func->return_types, ruyi_value_int32(return_type.ir_type));
    // detail
    ruyi_vector_add(func->return_types, ruyi_value_ptr(return_type.detail.uniptr));
}

ruyi_symtab_type ruyi_symtab_type_create(ruyi_ir_type ir_type, void *detail) {
    ruyi_symtab_type type;
    type.ir_type = ir_type;
    type.detail.uniptr = detail;
    return type;
}

void ruyi_symtab_type_destroy(ruyi_symtab_type type) {
    switch (type.ir_type) {
        case Ruyi_ir_type_Object:
            ruyi_symtab_type_object_destroy(type.detail.object);
            break;
        case Ruyi_ir_type_Array:
            ruyi_symtab_type_array_destroy(type.detail.array);
            break;
        case Ruyi_ir_type_Tuple:
            ruyi_symtab_type_tuple_destroy(type.detail.tuple);
            break;
        case Ruyi_ir_type_Map:
            ruyi_symtab_type_map_destroy(type.detail.map);
            break;
        case Ruyi_ir_type_Function:
            ruyi_symtab_type_func_destroy(type.detail.func);
            break;
        default:
            // do nothing for uniptr
            break;
    }
}

ruyi_symtab_type_object* ruyi_symtab_type_object_create(const ruyi_unicode_string *name) {
    ruyi_symtab_type_object * object;
    assert(name);
    object = (ruyi_symtab_type_object*) ruyi_mem_alloc(sizeof(ruyi_symtab_type_object));
    object->type = Ruyi_ir_type_Object;
    object->name = ruyi_unicode_string_copy_from(name);
    return object;
}

void ruyi_symtab_type_object_destroy(ruyi_symtab_type_object *object) {
    assert(object);
    if (object->name) {
        ruyi_unicode_string_destroy(object->name);
        object->name = NULL;
    }
    ruyi_mem_free(object);
}

ruyi_symtab_type_array* ruyi_symtab_type_array_create(UINT16 dims, ruyi_symtab_type type) {
    ruyi_symtab_type_array * array = (ruyi_symtab_type_array*)ruyi_mem_alloc(sizeof(ruyi_symtab_type_array));
    array->dims = dims;
    array->type = type;
    return array;
}

void ruyi_symtab_type_array_destroy(ruyi_symtab_type_array *array) {
    assert(array);
    ruyi_symtab_type_destroy(array->type);
    ruyi_mem_free(array);
}

ruyi_symtab_type_tuple* ruyi_symtab_type_tuple_create(UINT32 count) {
    ruyi_symtab_type_tuple * tuple = (ruyi_symtab_type_tuple*) ruyi_mem_alloc(sizeof(ruyi_symtab_type_tuple));
    tuple->count = count;
    tuple->types = (ruyi_symtab_type*)ruyi_mem_alloc(tuple->count * sizeof(ruyi_symtab_type));
    memset(tuple->types, 0, tuple->count * sizeof(ruyi_symtab_type));
    return tuple;
}

void ruyi_symtab_type_tuple_set(ruyi_symtab_type_tuple *tuple, UINT32 pos, ruyi_symtab_type type) {
    assert(tuple);
    assert(pos < tuple->count);
    tuple->types[pos] = type;
}

void ruyi_symtab_type_tuple_destroy(ruyi_symtab_type_tuple *tuple) {
    int i;
    if (!tuple) {
        return;
    }
    if (tuple->types) {
        for (i = 0; i < tuple->count; i++) {
            if (tuple->types[i].ir_type) {
                ruyi_symtab_type_destroy(tuple->types[i]);
            }
        }
        ruyi_mem_free(tuple->types);
    }
    ruyi_mem_free(tuple);
}

ruyi_symtab_type_map* ruyi_symtab_type_map_create(ruyi_symtab_type key_type, ruyi_symtab_type value_type) {
    ruyi_symtab_type_map * map = (ruyi_symtab_type_map*) sizeof(ruyi_symtab_type_map);
    map->key = key_type; // deep copy?
    map->value = key_type; // deep copy?
    return map;
}

void ruyi_symtab_type_map_destroy(ruyi_symtab_type_map *map) {
    if (!map) {
        return;
    }
    ruyi_symtab_type_destroy(map->key);
    ruyi_symtab_type_destroy(map->value);
    ruyi_mem_free(map);
}

ruyi_symtab_type_func* ruyi_symtab_type_func_create(ruyi_symtab_type_tuple *return_types, ruyi_symtab_type_tuple *parameter_types) {
    ruyi_symtab_type_func * func = (ruyi_symtab_type_func*) ruyi_mem_alloc(sizeof(ruyi_symtab_type_func));
    func->parameter_types = parameter_types;
    func->return_types = return_types;
    return func;
}

void ruyi_symtab_type_func_destroy(ruyi_symtab_type_func *func) {
    if (!func) {
        return;
    }
    if (func->parameter_types) {
        ruyi_symtab_type_tuple_destroy(func->parameter_types);
    }
    if (func->return_types) {
        ruyi_symtab_type_tuple_destroy(func->return_types);
    }
    ruyi_mem_free(func);
}
