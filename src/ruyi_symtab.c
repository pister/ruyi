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
    table->index2var = ruyi_vector_create();        // type of item: ruyi_symtab_var
    return table;
}

static
void index_hashtable_destroy(ruyi_symtab_index_hashtable *table) {
    INT32 i;
    INT32 len;
    ruyi_value value;
    ruyi_symtab_variable *var;
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
            var = (ruyi_symtab_variable *)value.data.ptr;
            assert(var);
            ruyi_unicode_string_destroy((ruyi_unicode_string*)var->name);
            ruyi_mem_free(var);
        }
        ruyi_vector_destroy(table->index2var);
    }
    ruyi_mem_free(table);
}

static
ruyi_error* index_hashtable_add(ruyi_symtab_index_hashtable *table, const ruyi_symtab_variable *var, UINT32 *out_index) {
    UINT32 index = 0;
    ruyi_value value;
    char name[NAME_BUF_LENGTH] = {0};
    ruyi_symtab_variable *var_copied;
    if (ruyi_hashtable_get(table->name2index, ruyi_value_unicode_str(var->name), &value)) {
        ruyi_unicode_string_encode_utf8_n(var->name, name, NAME_BUF_LENGTH-1);
        return ruyi_error_syntax("duplicated var define: %s", name);
    }
    
    index = ruyi_vector_length(table->index2var);

    var_copied = ruyi_mem_alloc(sizeof(ruyi_symtab_variable));
    var_copied->type = var->type;
    var_copied->var_size = var->var_size;
    var_copied->name = ruyi_unicode_string_copy_from(var->name);
    var_copied->index = index;
    var_copied->scope_type = var->scope_type;
    
    ruyi_vector_add(table->index2var, ruyi_value_ptr(var_copied));
    ruyi_hashtable_put(table->name2index, ruyi_value_unicode_str(var_copied->name), ruyi_value_uint32(index));
    if (out_index) {
        *out_index = index;
    }
    return NULL;
}

static
BOOL index_hashtable_get_by_name(const ruyi_symtab_index_hashtable *table, const ruyi_unicode_string* name, ruyi_symtab_variable *out_var) {
    ruyi_value index_value;
    ruyi_value item_value;
    const ruyi_symtab_variable* var;
    assert(out_var);
    if (!ruyi_hashtable_get(table->name2index, ruyi_value_unicode_str((ruyi_unicode_string*)name), &index_value)) {
        return FALSE;
    }
    if (!ruyi_vector_get(table->index2var, index_value.data.uint32_value, &item_value)) {
        return FALSE;
    }
    var = (ruyi_symtab_variable* )item_value.data.ptr;
    assert(var);
    out_var->index = var->index;
    out_var->type = var->type;
    out_var->var_size = var->var_size;
    out_var->scope_type = var->scope_type;
    // name will not be returned because caller knows the name.
    return TRUE;
}

ruyi_symtab* ruyi_symtab_create(void) {
    ruyi_symtab *symtab = (ruyi_symtab *)ruyi_mem_alloc(sizeof(ruyi_symtab));
    symtab->global_variables = index_hashtable_create();
    symtab->functions = ruyi_hashtable_create();
    symtab->cp = ruyi_symtab_constants_pool_create();
    return symtab;
}

void ruyi_symtab_destroy(ruyi_symtab *symtab) {
    if (NULL == symtab) {
        return;
    }
    if (symtab->cp) {
        ruyi_symtab_constants_pool_destroy(symtab->cp);
    }
    if (symtab->functions) {
        // TODO free data
        ruyi_hashtable_destroy(symtab->functions);
    }
    if (symtab->global_variables) {
        index_hashtable_destroy(symtab->global_variables);
    }
    ruyi_mem_free(symtab);
}

ruyi_error* ruyi_symtab_add_global_var(ruyi_symtab *symtab, const ruyi_symtab_variable *var, UINT32 *out_index) {
    assert(symtab);
    return index_hashtable_add(symtab->global_variables, var, out_index);
}

BOOL ruyi_symtab_get_global_var_by_name(const ruyi_symtab *symtab, const ruyi_unicode_string *name, ruyi_symtab_variable *out_var) {
    assert(symtab);
    return index_hashtable_get_by_name(symtab->global_variables, name, out_var);
}

ruyi_symtab_function_define* ruyi_symtab_function_create(ruyi_symtab *symtab, const ruyi_unicode_string *name) {
    ruyi_symtab_function_define *func = (ruyi_symtab_function_define*)ruyi_mem_alloc(sizeof(ruyi_symtab_function_define));
    func->name = name;
    if (name) {
        func->anonymous = FALSE;
    } else {
        func->anonymous = TRUE;
    }
    func->symtab = symtab;
    func->func_symtab_scope = ruyi_symtab_function_scope_create();
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
    if (func->func_symtab_scope) {
        ruyi_symtab_function_scope_destroy(func->func_symtab_scope);
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

ruyi_error* ruyi_symtab_function_add_arg(ruyi_symtab_function_define* func, const ruyi_symtab_variable *var) {
    UINT32 index;
    assert(func);
    if (func->args_types == NULL) {
        func->args_types = ruyi_vector_create();
    }
    // name
    ruyi_vector_add(func->args_types, ruyi_value_unicode_str(var->name));
    // ir_type
    ruyi_vector_add(func->args_types, ruyi_value_int32(var->type.ir_type));
    // detail
    ruyi_vector_add(func->args_types, ruyi_value_ptr(var->type.detail.uniptr));
    // create name ==> index mapping
    
    return ruyi_symtab_function_scope_add_var(func->func_symtab_scope, var, NULL);
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

// ================== ruyi_function_scope ==================

ruyi_function_scope* ruyi_symtab_function_scope_create(void) {
    ruyi_function_scope *function_scope = (ruyi_function_scope *)ruyi_mem_alloc(sizeof(ruyi_function_scope));
    function_scope->block_scope_stack = ruyi_list_create();
    // first enter function
    ruyi_symtab_function_scope_enter(function_scope);
    return function_scope;
}

void ruyi_symtab_function_scope_destroy(ruyi_function_scope *function_scope) {
    ruyi_list_item *item;
    ruyi_symtab_index_hashtable *table;
    if (NULL == function_scope) {
        return;
    }
    if (function_scope->block_scope_stack) {
        item = function_scope->block_scope_stack->first;
        while (item != function_scope->block_scope_stack->last) {
            table = (ruyi_symtab_index_hashtable *)item->value.data.ptr;
            if (table) {
                index_hashtable_destroy(table);
            }
            item = item->next;
        }
        ruyi_list_destroy(function_scope->block_scope_stack);
    }
    ruyi_mem_free(function_scope);
}

void ruyi_symtab_function_scope_enter(ruyi_function_scope* scope) {
    // lazy create ...
    ruyi_list_add_last(scope->block_scope_stack, ruyi_value_ptr(NULL));
}

void ruyi_symtab_function_scope_leave(ruyi_function_scope* scope) {
    ruyi_value last_value;
    ruyi_symtab_index_hashtable *table;
    assert(scope);
    if (!ruyi_list_remove_last(scope->block_scope_stack, &last_value)) {
        return;
    }
    table = (ruyi_symtab_index_hashtable *)last_value.data.ptr;
    if (table) {
        index_hashtable_destroy(table);
    }
}

ruyi_error* ruyi_symtab_function_scope_add_var(ruyi_function_scope* scope, const ruyi_symtab_variable *var, UINT32 *out_index) {
    ruyi_symtab_index_hashtable *table;
    ruyi_list_item *item;
    assert(scope);
    item = scope->block_scope_stack->last;
    if (!item) {
        // NOT call: ruyi_symtab_function_scope_enter()
        assert(0);
    }
    table = (ruyi_symtab_index_hashtable *)item->value.data.ptr;
    if (!table) {
        table = index_hashtable_create();
        item->value.data.ptr = table;
    }
    return index_hashtable_add(table, var, out_index);
}

BOOL ruyi_symtab_function_scope_get(ruyi_function_scope* scope, const ruyi_unicode_string *name, ruyi_symtab_variable *out_var) {
    ruyi_symtab_index_hashtable *table;
    ruyi_list_item *item;
    assert(scope);
    item = scope->block_scope_stack->last;
    while (item) {
        table = (ruyi_symtab_index_hashtable *)item->value.data.ptr;
        if (table && index_hashtable_get_by_name(table, name, out_var)) {
            return TRUE;
        }
        item = item->prev;
    }
    return FALSE;
}
// =====================================================================

ruyi_symtab_constant *ruyi_symtab_constant_int64(UINT32 index, INT64 value) {
    ruyi_symtab_constant * c = (ruyi_symtab_constant*) ruyi_mem_alloc(sizeof(ruyi_symtab_constant));
    c->index = index;
    c->type = Ruyi_ir_type_Int64;
    c->data.int64_value = value;
    return c;
}
ruyi_symtab_constant *ruyi_symtab_constant_float64(UINT32 index, FLOAT64 value) {
    ruyi_symtab_constant * c = (ruyi_symtab_constant*) ruyi_mem_alloc(sizeof(ruyi_symtab_constant));
    c->index = index;
    c->type = Ruyi_ir_type_Float64;
    c->data.float64_value = value;
    return c;
}
ruyi_symtab_constant *ruyi_symtab_constant_unicode(UINT32 index, const ruyi_unicode_string* value) {
    ruyi_symtab_constant * c = (ruyi_symtab_constant*) ruyi_mem_alloc(sizeof(ruyi_symtab_constant));
    c->index = index;
    c->type = Ruyi_ir_type_String;
    if (value) {
        c->data.uncode_str = ruyi_unicode_string_copy_from(value);
    } else {
        c->data.uncode_str = NULL;
    }
    return c;
}

void ruyi_symtab_constant_destroy(ruyi_symtab_constant *c) {
    if (!c) {
        return;
    }
    if (c->type == Ruyi_ir_type_String) {
        ruyi_unicode_string * str = c->data.uncode_str;
        if (str) {
            ruyi_unicode_string_destroy(str);
        }
    }
    ruyi_mem_free(c);
}

// =====================================================================

ruyi_symtab_constants_pool * ruyi_symtab_constants_pool_create(void) {
    ruyi_symtab_constants_pool * cp = (ruyi_symtab_constants_pool*)ruyi_mem_alloc(sizeof(ruyi_symtab_constants_pool));
    // lazy create
    cp->int642index = NULL;
    cp->float642index = NULL;
    cp->unicode2index = NULL;
    cp->index2value = NULL;
    return cp;
}

void ruyi_symtab_constants_pool_destroy(ruyi_symtab_constants_pool* cp) {
    UINT32 i, len;
    ruyi_value value;
    ruyi_symtab_constant *const_value;
    if (!cp) {
        return;
    }
    if (cp->int642index) {
        ruyi_hashtable_destroy(cp->int642index);
    }
    if (cp->float642index) {
        ruyi_hashtable_destroy(cp->float642index);
    }
    if (cp->unicode2index) {
        ruyi_hashtable_destroy(cp->unicode2index);
    }
    if (cp->index2value) {
        len = ruyi_vector_length(cp->index2value);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(cp->index2value, i, &value);
            const_value = (ruyi_symtab_constant *)value.data.ptr;
            if (const_value) {
                ruyi_symtab_constant_destroy(const_value);
            }
        }
        ruyi_vector_destroy(cp->index2value);
    }
    ruyi_mem_free(cp);
}

UINT32 ruyi_symtab_constants_pool_get_or_add_int64(ruyi_symtab_constants_pool *cp, INT64 value) {
    ruyi_value found_value;
    UINT32 index;
    assert(cp);
    if (!cp->index2value) {
        cp->index2value = ruyi_vector_create();
    }
    if (!cp->int642index) {
        cp->int642index = ruyi_hashtable_create();
    }
    if (ruyi_hashtable_get(cp->int642index, ruyi_value_int64(value), &found_value)) {
        return ((ruyi_symtab_constant *)found_value.data.ptr)->index;
    }
    index = ruyi_vector_length(cp->index2value);
    ruyi_hashtable_put(cp->int642index, ruyi_value_int64(value), ruyi_value_ptr(ruyi_symtab_constant_int64(index, value)));
    return index;
}

UINT32 ruyi_symtab_constants_pool_get_or_add_float64(ruyi_symtab_constants_pool *cp, FLOAT64 value) {
    ruyi_value found_value;
    UINT32 index;
    assert(cp);
    if (!cp->index2value) {
        cp->index2value = ruyi_vector_create();
    }
    if (!cp->float642index) {
        cp->float642index = ruyi_hashtable_create();
    }
    if (ruyi_hashtable_get(cp->float642index, ruyi_value_float64(value), &found_value)) {
        return ((ruyi_symtab_constant *)found_value.data.ptr)->index;
    }
    index = ruyi_vector_length(cp->index2value);
    ruyi_hashtable_put(cp->float642index, ruyi_value_float64(value), ruyi_value_ptr(ruyi_symtab_constant_float64(index, value)));
    return index;
}

UINT32 ruyi_symtab_constants_pool_get_or_add_unicode(ruyi_symtab_constants_pool *cp, const ruyi_unicode_string *value) {
    ruyi_value found_value;
    UINT32 index;
    ruyi_symtab_constant *c;
    assert(cp);
    if (!cp->index2value) {
        cp->index2value = ruyi_vector_create();
    }
    if (!cp->unicode2index) {
        cp->unicode2index = ruyi_hashtable_create();
    }
    if (ruyi_hashtable_get(cp->unicode2index, ruyi_value_unicode_str(value), &found_value)) {
        return ((ruyi_symtab_constant *)found_value.data.ptr)->index;
    }
    index = ruyi_vector_length(cp->index2value);
    c = ruyi_symtab_constant_unicode(index, value);
    ruyi_hashtable_put(cp->unicode2index, ruyi_value_unicode_str(c->data.uncode_str), ruyi_value_ptr(c));
    return index;
}

