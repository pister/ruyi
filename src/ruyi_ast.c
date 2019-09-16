//
//  ruyi_ast.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/29.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_ast.h"
#include "ruyi_mem.h"

void ruyi_ast_destroy(ruyi_ast *ast) {
    UINT32 i, len;
    ruyi_value sub_ast_value;
    ruyi_ast *sub_ast_ptr;
    assert(ast);
    // TODO
    switch (ast->type) {
        case Ruyi_at_global_declaration:
        
        break;
        case Ruyi_at_root:
        
        default:
        break;
    }
    if (ast->child_asts) {
        i = 0;
        for (len = ruyi_vector_length(ast->child_asts); i < len; i++) {
            ruyi_vector_get(ast->child_asts, i, &sub_ast_value);
            sub_ast_ptr = (ruyi_ast *)sub_ast_value.data.ptr;
            if (sub_ast_ptr) {
                ruyi_ast_destroy(sub_ast_ptr);
            }
        }
        ruyi_vector_destroy(ast->child_asts);
        ast->child_asts = NULL;
    }
    // destory self
    ruyi_mem_free(ast);
}

ruyi_ast * ruyi_ast_create(ruyi_ast_type type) {
    ruyi_ast *ast = ruyi_mem_alloc(sizeof(ruyi_ast));
    ast->type = type;
    ast->data.int64_value = 0;
    ast->child_asts = NULL;
    return ast;
}

void ruyi_ast_add_child(ruyi_ast *ast, ruyi_ast *child) {
    assert(ast);
    if (!ast->child_asts) {
        ast->child_asts = ruyi_vector_create();
    }
    ruyi_vector_add(ast->child_asts, ruyi_value_ptr(child));
}

ruyi_ast * ruyi_ast_get_child(ruyi_ast *ast, UINT32 index) {
    ruyi_value child_ptr;
    assert(ast);
    if (!ast->child_asts) {
        return NULL;
    }
    if (!ruyi_vector_get(ast->child_asts, index, &child_ptr)) {
        return NULL;
    }
    return (ruyi_ast *)child_ptr.data.ptr;
}

