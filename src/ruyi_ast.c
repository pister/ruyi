//
//  ruyi_ast.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/29.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_ast.h"
#include "ruyi_mem.h"


// momery leak
void ruyi_ast_destroy(ruyi_ast *ast) {
    UINT32 i, len;
    ruyi_value sub_ast_value;
    ruyi_ast *sub_ast_ptr;
    ruyi_unicode_string * ustr;
    if(ast == NULL) {
        return;
    }
    switch (ast->adt_type) {
    case Ruyi_adt_unicode_str:
            ustr = (ruyi_unicode_string *)ast->data.ptr_value;
            if (ustr) {
                ruyi_unicode_string_destroy(ustr);
                ast->data.ptr_value = NULL;
            }
        break;
    default:
        break;
    }
    if (ast->child_asts) {
        for ((void)(len = ruyi_vector_length(ast->child_asts)), i = 0; i < len; i++) {
            ruyi_vector_get(ast->child_asts, i, &sub_ast_value);
            sub_ast_ptr = (ruyi_ast *)sub_ast_value.data.ptr;
            if (sub_ast_ptr) {
                ruyi_ast_destroy(sub_ast_ptr);
            }
        }
        ruyi_vector_destroy(ast->child_asts);
        ast->child_asts = NULL;
    }
    // destroy self
    ruyi_mem_free(ast);
}

void ruyi_ast_destroy_without_child(ruyi_ast *ast) {
    ruyi_unicode_string * ustr;
    if(ast == NULL) {
        return;
    }
    switch (ast->adt_type) {
        case Ruyi_adt_unicode_str:
            ustr = (ruyi_unicode_string *)ast->data.ptr_value;
            if (ustr) {
                ruyi_unicode_string_destroy(ustr);
                ast->data.ptr_value = NULL;
            }
            break;
        default:
            break;
    }
    if (ast->child_asts) {
        ruyi_vector_destroy(ast->child_asts);
        ast->child_asts = NULL;
    }
    // destory self
    ruyi_mem_free(ast);
}

ruyi_ast * ruyi_ast_create(ruyi_ast_type type) {
    ruyi_ast *ast = ruyi_mem_alloc(sizeof(ruyi_ast));
    ast->type = type;
    ast->adt_type = Ruyi_adt_value;
    ast->data.int64_value = 0;
    ast->child_asts = NULL;
    return ast;
}

ruyi_ast * ruyi_ast_create_by_token_type(ruyi_ast_type type, ruyi_token_type token_type) {
    ruyi_ast *ast = ruyi_ast_create(type);
    ast->data.int64_value = token_type;
    return ast;
}

ruyi_ast * ruyi_ast_create_with_unicode(ruyi_ast_type type, ruyi_unicode_string *str) {
    ruyi_ast *ast = ruyi_ast_create(type);
    ast->adt_type = Ruyi_adt_unicode_str;
    ast->data.ptr_value = ruyi_unicode_string_copy_from(str);
    return ast;
}

void ruyi_ast_add_child(ruyi_ast *ast, ruyi_ast *child) {
    assert(ast);
    if (!ast->child_asts) {
        ast->child_asts = ruyi_vector_create();
    }
    ruyi_vector_add(ast->child_asts, ruyi_value_ptr(child));
}

ruyi_ast * ruyi_ast_get_child(const ruyi_ast *ast, UINT32 index) {
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


UINT32 ruyi_ast_child_length(const ruyi_ast *ast) {
    assert(ast);
    if (ast->child_asts == NULL) {
        return 0;
    }
    return ast->child_asts->len;
}
