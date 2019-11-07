//
//  ruyi_symtab.h
//  ruyi
//
//  Created by Songli Huang on 2019/10/31.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_symtab_h
#define ruyi_symtab_h

#include "ruyi_vector.h"
#include "ruyi_hashtable.h"
#include "ruyi_unicode.h"




typedef struct {
    ruyi_vector *block_scope_stack;
} ruyi_function_scope;

ruyi_function_scope* ruyi_symtab_function_scope_create(void);
void ruyi_symtab_function_scope_destroy(ruyi_function_scope *function_scope);

// symbol table
// variables table (scope): global variables and local variables.
// function table (global): name -> function.
// constants (global): iteral: integer, float, strings.
// class (global)
// interface (global)
typedef struct {
    ruyi_function_scope *function_scope;
    ruyi_hashtable *functions;
    ruyi_hashtable *constants;
} ruyi_symtab;

ruyi_symtab* ruyi_symtab_create(void);

void ruyi_symtab_destroy(ruyi_symtab *symtab);

void ruyi_symtab_enter_scope(ruyi_symtab *symtab);

void ruyi_symtab_leave_scope(ruyi_symtab *symtab);

void ruyi_symtab_enter_func(ruyi_symtab *symtab, ruyi_unicode_string* name);



#endif /* ruyi_symtab_h */
