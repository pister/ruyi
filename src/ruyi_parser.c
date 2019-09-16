//
//  ruyi_parser.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/29.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_parser.h"
#include "ruyi_basics.h"
#include "ruyi_lexer.h"
#include "ruyi_vector.h"
#include "ruyi_error.h"

static ruyi_ast * create_ast_by_consume_token_string(ruyi_lexer_reader *reader, ruyi_ast_type type) {
    ruyi_ast * ret;
    ruyi_token *token = ruyi_lexer_reader_next_token(reader);
    ret = ruyi_ast_create_with_unicode(Ruyi_at_var_declaration, token->value.str_value);
    ruyi_lexer_token_destroy(token);
    return ret;
}

static
ruyi_error* expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    
    return NULL;
}

static
ruyi_error* array_initializer(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    
    return NULL;
}

static
ruyi_error* variable_initializer(ruyi_lexer_reader *reader, ruyi_ast **out_ast, ruyi_token_pos pos) {
    // <variable initializer> ::= <expression> | <array initializer>
    ruyi_ast *ast = NULL;
    ruyi_error* err;
    if ((err = expression(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = array_initializer(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    return ruyi_error_make_by_pos(Ruyi_et_Parser, "miss expression or array after '='", pos);
}

static
ruyi_error* variable_declaration_tail(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable declaration tail> ::= (LBRACKET RBRACKET) * IDENTITY (ASSIGN <variable initializer>) ?
    ruyi_error* err;
    ruyi_token token1;
    ruyi_token token2;
    ruyi_ast *var_declare_ast;
    ruyi_ast *ast_var_init;
    UINT32 dims = 0;
    while (TRUE) {
        if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACKET, &token1)) {
            if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACKET, &token2)) {
                // make error
                return ruyi_error_make(Ruyi_et_Parser, "array variable define miss ']' after '['", &token2);
            }
            dims++;
            continue;
        }
        break;
    }
    if (Ruyi_tt_IDENTITY != ruyi_lexer_reader_peek_token_type(reader)) {
        *out_ast = NULL;
        return NULL;
    }
    var_declare_ast = create_ast_by_consume_token_string(reader, Ruyi_at_var_declaration);
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_ASSIGN, &token1)) {
        *out_ast = var_declare_ast;
        return NULL;
    }
    ruyi_lexer_reader_consume_token(reader); // ASSIGN
    if ((err = variable_initializer(reader, &ast_var_init, ruyi_token_pos_make(&token1))) != NULL) {
        return err;
    }
    ruyi_ast_add_child(var_declare_ast, ast_var_init);
    *out_ast = var_declare_ast;
    return NULL;
}

static
ruyi_error* variable_declaration(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable declaration> ::= KW_VAR <variable declaration tail>
    ruyi_error* err;
    ruyi_ast *var_declare_ast;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_VAR, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = variable_declaration_tail(reader, &var_declare_ast)) != NULL) {
        return err;
    }
    *out_ast = var_declare_ast;
    return NULL;
}

static
ruyi_error* global_declaration(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <global declaration> ::= <variable declaration> | <function declaration> | <class declaration> | <interface declaration> | <constant declaration> | <statement ends>
    ruyi_ast *ast = NULL;
    ruyi_error* err;
    err = variable_declaration(reader, &ast);
    if (err != NULL) {
        return err;
    }
    if (ast) {
        *out_ast = ast;
        return NULL;
    }
    // TODO
    // err = function_declaration(reader, &ast);
    // ...
    
    return NULL;
}

static
ruyi_error* global_declarations(ruyi_lexer_reader *reader, ruyi_ast *parent) {
    // <global declarations> ::= <global declaration> *
    ruyi_ast *out_ast = NULL;
    ruyi_error* err;
    while (TRUE) {
        err = global_declaration(reader, &out_ast);
        if (err != NULL) {
            return err;
        }
        if (out_ast) {
            ruyi_ast_add_child(parent, out_ast);
        } else {
            // End Of Next global_declaration
            break;
        }
    }
    return NULL;
}

static
ruyi_error* compilation_unit(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <root> ::= <global declarations>?
    *out_ast = ruyi_ast_create(Ruyi_at_root);
    ruyi_error* err;
    err = global_declarations(reader, *out_ast);
    return err;
}

ruyi_error* ruyi_parse_ast(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    return compilation_unit(reader, out_ast);
}
