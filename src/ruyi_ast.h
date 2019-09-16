//
//  ruyi_ast.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/29.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_ast_h
#define ruyi_ast_h

#include "ruyi_basics.h"
#include "ruyi_vector.h"

typedef enum {
    Ruyi_at_root,
    Ruyi_at_global_declaration,
} ruyi_ast_type;

struct _ruyi_ast;
typedef struct _ruyi_ast {
    ruyi_ast_type type;
    union {
        WIDE_CHAR char_value;
        UINT32 int32_value;
        UINT64 int64_value;
        void* ptr_value;
        double double_value;
    } data;
    ruyi_vector* child_asts;
} ruyi_ast;

ruyi_ast * ruyi_ast_create(ruyi_ast_type type);

void ruyi_ast_add_child(ruyi_ast *ast, ruyi_ast *child);

 ruyi_ast * ruyi_ast_get_child(ruyi_ast *ast, UINT32 index);


void ruyi_ast_destroy(ruyi_ast *ast);

#endif /* ruyi_ast_h */
