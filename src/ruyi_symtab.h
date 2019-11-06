//
//  ruyi_symtab.h
//  ruyi
//
//  Created by Songli Huang on 2019/10/31.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_symtab_h
#define ruyi_symtab_h

#include "ruyi_unicode.h"

// symbol table
// variables table (scope): global variables and local variables.
// function table (global): name -> function.
// const pool (global): iteral: integer, float, strings.
// class
typedef struct {
    
} ruyi_symtab;

ruyi_symtab* ruyi_symtab_create(void);

ruyi_symtab* ruyi_symtab_destroy(ruyi_symtab *symtab);

void ruyi_symtab_enter_scope(ruyi_symtab *symtab);

void ruyi_symtab_leave_scope(ruyi_symtab *symtab);

void ruyi_symtab_enter_func(ruyi_symtab *symtab, ruyi_unicode_string* name);


void ruyi_symtab_add(ruyi_symtab *symtab, ruyi_unicode_string* name);

#endif /* ruyi_symtab_h */
