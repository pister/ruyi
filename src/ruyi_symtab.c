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
    symtab->global_variables = ruyi_hashtable_create();
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
        ruyi_hashtable_destroy(symtab->global_variables);
    }
    ruyi_mem_free(symtab);
}

void ruyi_symtab_enter_scope(ruyi_symtab *symtab) {
    
}

void ruyi_symtab_leave_scope(ruyi_symtab *symtab) {
    
}
