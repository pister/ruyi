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
            ruyi_unicode_string_destroy(var->name);
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

