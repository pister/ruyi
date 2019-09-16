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


static
ruyi_error* variable_declaration_tail(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable declaration tail> ::= (LBRACKET RBRACKET) * IDENTITY (ASSIGN <variable initializer>) ?
    
    return NULL;
}

static
ruyi_error* variable_declaration(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable declaration> ::= KW_VAR <variable declaration tail>
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_VAR)) {
        
    }
    *out_ast = NULL;
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
