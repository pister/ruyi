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
    ret = ruyi_ast_create_with_unicode(type, token->value.str_value);
    ruyi_lexer_token_destroy(token);
    return ret;
}


static
ruyi_error* assignment_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* conditional_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    return NULL;
}

static
ruyi_error* field_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* array_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* name(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <name> ::= IDENTITY (DOT IDENTITY) *
    ruyi_error *err;
    ruyi_ast *name = NULL;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        *out_ast = NULL;
        return NULL;
    }
    name = create_ast_by_consume_token_string(reader, Ruyi_at_name);
    while (TRUE) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_DOT, NULL)) {
            break;
        }
        if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
            err = ruyi_error_by_parser(reader, "need an identifier after '.'");
            goto name_on_error;
        }
        ruyi_ast_add_child(name, create_ast_by_consume_token_string(reader, Ruyi_at_name_part));
    }
    *out_ast = name;
    return NULL;
name_on_error:
    if (name) {
        ruyi_ast_destroy(name);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* literal(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <literal> ::= INTEGER | FLOAT | KW_TRUE | KW_FALSE | RUNE | STRING | KW_NULL
    ruyi_ast *ast = NULL;
    ruyi_token *token = ruyi_lexer_reader_next_token(reader);
    switch (token->type) {
        case Ruyi_tt_INTEGER:
            ast = ruyi_ast_create(Ruyi_at_integer);
            ast->data.int64_value = token->value.int_value;
            break;
        case Ruyi_tt_FLOAT:
            ast = ruyi_ast_create(Ruyi_at_float);
            ast->data.float_value = token->value.float_value;
            break;
        case Ruyi_tt_KW_TRUE:
            ast = ruyi_ast_create(Ruyi_at_bool);
            ast->data.int64_value = TRUE;
            break;
        case Ruyi_tt_KW_FALSE:
            ast = ruyi_ast_create(Ruyi_at_bool);
            ast->data.int64_value = FALSE;
            break;
        case Ruyi_tt_CHAR:
            ast = ruyi_ast_create(Ruyi_at_rune);
            ast->data.int64_value = token->value.int_value;
            break;
        case Ruyi_tt_STRING:
            ast = ruyi_ast_create_with_unicode(Ruyi_at_string, token->value.str_value);
            break;
        case Ruyi_tt_KW_NULL:
            ast = ruyi_ast_create(Ruyi_at_null);
            break;
        default:
            ruyi_lexer_reader_push_back(reader, token);
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <type> ::= <primitive type> | <reference type>

    return NULL;
}

static
ruyi_error* array_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array type> ::= LBRACKET RBRACKET <type>

    return NULL;
}


static
ruyi_error* map_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <map type> ::= LBRACKET IDENTITY RBRACKET <type>

    return NULL;
}

static
ruyi_error* function_invocation(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <function invocation> ::= <name> ( <argument list>? ) | <primary> DOT IDENTITY ( <argument list>? )
    
    return NULL;
}

static
ruyi_error* instance_creation(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <instance creation> ::= IDENTITY LBRACE (IDENTITY COLON <expression> (COMMA IDENTITY COLON <expression>)*)?  RBRACE
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *name_ast;
    ruyi_token *name_token;
    ruyi_ast *property_name_ast = NULL;
    ruyi_ast *property_expr_ast;
    ruyi_ast *property_ast;
    
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        *out_ast = NULL;
        return NULL;
    }
    name_token = ruyi_lexer_reader_next_token(reader);
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_LBRACE) {
        ruyi_lexer_reader_push_back(reader, name_token);
        *out_ast = NULL;
        return NULL;
    }
    
    name_ast = ruyi_ast_create_with_unicode(Ruyi_at_name, name_token->value.str_value);
    ruyi_lexer_token_destroy(name_token);
    ast = ruyi_ast_create(Ruyi_at_instance_creation);
    ruyi_ast_add_child(ast, name_ast);
   
    if (ruyi_lexer_reader_peek_token_type(reader) == Ruyi_tt_IDENTITY) {
        property_name_ast = create_ast_by_consume_token_string(reader, Ruyi_at_name);
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COLON, NULL)) {
            err = ruyi_error_by_parser(reader, "need ':' after property name when create instance");
            goto instance_creation_on_error;
        }
        if ((err = expression(reader, &property_expr_ast)) != NULL) {
            goto instance_creation_on_error;
        }
        property_ast = ruyi_ast_create(Ruyi_at_property);
        ruyi_ast_add_child(property_ast, property_name_ast);
        ruyi_ast_add_child(property_ast, property_expr_ast);
        ruyi_ast_add_child(ast, property_ast);
        while (TRUE) {
            if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
                break;
            }
            property_name_ast = create_ast_by_consume_token_string(reader, Ruyi_at_name);
            if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COLON, NULL)) {
                err = ruyi_error_by_parser(reader, "need ':' after property name when create instance");
                goto instance_creation_on_error;
            }
            if ((err = expression(reader, &property_expr_ast)) != NULL) {
                goto instance_creation_on_error;
            }
            property_ast = ruyi_ast_create(Ruyi_at_property);
            ruyi_ast_add_child(property_ast, property_name_ast);
            ruyi_ast_add_child(property_ast, property_expr_ast);
            ruyi_ast_add_child(ast, property_ast);
        }
    }
    
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACE, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '}' when create instance");
        goto instance_creation_on_error;
    }
    *out_ast = ast;
    return NULL;
    
instance_creation_on_error:
    if (ast) {
        ruyi_ast_destroy(ast);
    }
    if (property_name_ast) {
        ruyi_ast_destroy(property_name_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* primary_no_new_collection(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <primary no new array> ::= <literal> | KW_THIS | LPARAN <expression> RPARAN  | <field access> | <function invocation> | <array access> | < instance creation>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = literal(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_THIS, NULL)) {
        ast = ruyi_ast_create(Ruyi_at_this);
        *out_ast = ast;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        if ((err = expression(reader, &ast)) != NULL) {
            return err;
        }
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
            return ruyi_error_by_parser(reader, "miss ')'");
        }
        if (ast != NULL) {
            *out_ast = ast;
            return NULL;
        }
    }
    if ((err = field_access(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = function_invocation(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = array_access(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = instance_creation(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* map_creation(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <map creation> ::= KW_MAP LPARAN <map type> RPARAN
    ruyi_error *err;
    ruyi_ast *ast_map_type = NULL;
    ruyi_ast *ast;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_MAP, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '(' after keyword 'map'");
        goto map_creation_on_error;
    }
    if ((err = map_type(reader, &ast_map_type)) != NULL) {
        goto map_creation_on_error;
    }
    if (ast_map_type == NULL) {
        err = ruyi_error_by_parser(reader, "miss map type when create map");
        goto map_creation_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ')' when create map");
        goto map_creation_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_map_creation);
    ruyi_ast_add_child(ast, ast_map_type);
    *out_ast = ast;
    return NULL;
map_creation_on_error:
    if (ast_map_type) {
        ruyi_ast_destroy(ast_map_type);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* array_creation_with_cap(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array creation with cap> ::= KW_ARRAY LPARAN <array type> COMMA <expression> (COMMA <expression>)? RPARAN
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_array_type = NULL;
    ruyi_ast *expr_len = NULL;
    ruyi_ast *expr_cap = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_ARRAY, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '(' after keyword 'array'");
        goto array_creation_with_cap_on_err;
    }
    if ((err = array_type(reader, &ast_array_type)) != NULL) {
        goto array_creation_with_cap_on_err;
    }
    if (ast_array_type == NULL) {
        err = ruyi_error_by_parser(reader, "miss array type when create array");
        goto array_creation_with_cap_on_err;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ',' when create array");
        goto array_creation_with_cap_on_err;
    }
    if ((err = expression(reader, &expr_len)) != NULL) {
        goto array_creation_with_cap_on_err;
    }
    if (expr_len == NULL) {
        err = ruyi_error_by_parser(reader, "miss length expression after ',' when create array");
        goto array_creation_with_cap_on_err;
    }
    ast = ruyi_ast_create(Ruyi_at_array_creation_with_cap);
    ruyi_ast_add_child(ast, ast_array_type);
    ruyi_ast_add_child(ast, expr_len);
    ast_array_type = NULL;
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
        if ((err = expression(reader, &expr_cap)) != NULL) {
            goto array_creation_with_cap_on_err;
        }
        if (expr_cap == NULL) {
            err = ruyi_error_by_parser(reader, "miss capacity expression after ',' when create array");
            goto array_creation_with_cap_on_err;
        }
        ruyi_ast_add_child(ast, expr_cap);
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ')' when create array");
        goto array_creation_with_cap_on_err;
    }
    *out_ast = ast;
    return NULL;
array_creation_with_cap_on_err:
    if (ast_array_type != NULL) {
        ruyi_ast_destroy(ast_array_type);
    }
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* array_creation_with_init(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array creation with init> ::= <array type> LBRACKET (<expression> (COMMA <expression>)*)? RBRACKET
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *ast_array_type = NULL;
    if ((err = array_type(reader, &ast_array_type)) != NULL) {
        return err;
    }
    if (ast_array_type == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACKET, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '[' when create array");
        goto array_creation_with_init_on_error;
    }
    if ((err = expression(reader, &expr_ast)) != NULL) {
        goto array_creation_with_init_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_array_creation_with_init);
    if (expr_ast != NULL) {
        ruyi_ast_add_child(ast, expr_ast);
        while (TRUE) {
            if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
                break;
            }
            if ((err = expression(reader, &expr_ast)) != NULL) {
                return err;
            }
            if (expr_ast == NULL) {
                err = ruyi_error_by_parser(reader, "miss expression after ','");
                goto array_creation_with_init_on_error;
            }
            ruyi_ast_add_child(ast, expr_ast);
        }
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACKET, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ']' when create array");
        goto array_creation_with_init_on_error;
    }
    *out_ast = ast;
    return NULL;
array_creation_with_init_on_error:
    if (ast) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* array_creation(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array creation> ::= <array creation with cap> | <array creation with init>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = array_creation_with_cap(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = array_creation_with_init(reader, &ast)) != NULL) {
        return err;
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* primary(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <primary> ::= <primary no new collection> | <array creation> | <map creation>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = primary_no_new_collection(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = array_creation(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = map_creation(reader, &ast)) != NULL) {
        return err;
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* field_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <field access> ::= <primary> DOT IDENTITY
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *primary_ast = NULL;
    if ((err = primary(reader, &primary_ast)) != NULL) {
        return err;
    }
    if (primary_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_DOT, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '.' when access field");
        goto field_access_on_error;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        err = ruyi_error_by_parser(reader, "need identifier after '.'");
        goto field_access_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_field_access);
    ruyi_ast_add_child(ast, primary_ast);
    ruyi_ast_add_child(ast, create_ast_by_consume_token_string(reader, Ruyi_at_name_part));
    *out_ast = ast;
    return NULL;
field_access_on_error:
    if (primary_ast != NULL) {
        ruyi_ast_destroy(primary_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* array_variable_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array variable access> ::= <name> LBRACKET <expression> RBRACKET
    ruyi_error *err;
    ruyi_ast *name_ast = NULL;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *ast;
    if ((err = name(reader, &name_ast)) != NULL) {
        return err;
    }
    if (name_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACKET, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '[' where access array");
        goto array_variable_access_on_error;
    }
    if ((err = expression(reader, &expr_ast)) != NULL) {
        goto array_variable_access_on_error;
    }
    if (expr_ast == NULL) {
        err = ruyi_error_by_parser(reader, "miss expression after '[' where access array");
        goto array_variable_access_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACKET, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ']' where access array");
        goto array_variable_access_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_array_variable_access);
    ruyi_ast_add_child(ast, name_ast);
    ruyi_ast_add_child(ast, expr_ast);
    *out_ast = ast;
    return NULL;
array_variable_access_on_error:
    if (name_ast != NULL) {
        ruyi_ast_destroy(name_ast);
    }
    if (expr_ast != NULL) {
        ruyi_ast_destroy(expr_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* variable_primary_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable primary access> ::= <primary no new collection> LBRACKET <expression> RBRACKET
    ruyi_error *err;
    ruyi_ast *primary_ast = NULL;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *ast;
    if ((err = primary_no_new_collection(reader, &primary_ast)) != NULL) {
        return err;
    }
    if (primary_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACKET, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '[' where access array");
        goto variable_primary_access_on_error;
    }
    if ((err = expression(reader, &expr_ast)) != NULL) {
        goto variable_primary_access_on_error;
    }
    if (expr_ast == NULL) {
        err = ruyi_error_by_parser(reader, "miss expression after '[' where access array");
        goto variable_primary_access_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACKET, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ']' where access array");
        goto variable_primary_access_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_array_primary_access);
    ruyi_ast_add_child(ast, primary_ast);
    ruyi_ast_add_child(ast, expr_ast);
    *out_ast = ast;
    return NULL;
variable_primary_access_on_error:
    if (primary_ast != NULL) {
        ruyi_ast_destroy(primary_ast);
    }
    if (expr_ast != NULL) {
        ruyi_ast_destroy(expr_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* array_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array access> ::= <array variable access> | <variable primary access>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = array_variable_access(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = variable_primary_access(reader, &ast)) != NULL) {
        return err;
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* left_hand_side(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    //  <left hand side> ::= <name> | <field access> | <array access>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = name(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = field_access(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = array_access(reader, &ast)) != NULL) {
        return err;
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* assignment_operator(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    //  <assignment operator> ::= ASSIGN | MUL_ASS | DIV_ASS | MOD_ASS | ADD_ASS | SUB_ASS | SHFT_LEFT_ASS | SHFT_RIGHT_ASS | BIT_AND_ASS | BIT_XOR_ASS | BIT_OR_ASS
    ruyi_token_type token_type = ruyi_lexer_reader_peek_token_type(reader);
    switch (token_type) {
        case Ruyi_tt_ASSIGN:
        case Ruyi_tt_MUL_ASS:
        case Ruyi_tt_DIV_ASS:
        case Ruyi_tt_MOD_ASS:
        case Ruyi_tt_ADD_ASS:
        case Ruyi_tt_SUB_ASS:
        case Ruyi_tt_SHFT_LEFT_ASS:
        case Ruyi_tt_SHFT_RIGHT_ASS:
        case Ruyi_tt_BIT_AND_ASS:
        case Ruyi_tt_BIT_XOR_ASS:
        case Ruyi_tt_BIT_OR_ASS:
            ruyi_lexer_reader_consume_token(reader);
            *out_ast = ruyi_ast_create_by_token_type(Ruyi_at_assign_operator, token_type);
            return NULL;
        default:
            *out_ast = NULL;
            return NULL;
    }
}

static
ruyi_error* assignment(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    //  <assignment> ::= <left hand side> <assignment operator> <assignment expression>
    ruyi_error *err;
    ruyi_ast *left_hand_side_ast = NULL;
    ruyi_ast *ass_oper_ast = NULL;
    ruyi_ast *ass_expr_ast = NULL;
    ruyi_ast *ast;
    if ((err = left_hand_side(reader, &left_hand_side_ast)) != NULL) {
        return err;
    }
    if (left_hand_side_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = assignment_operator(reader, &ass_oper_ast)) != NULL) {
        goto assignment_on_error;
    }
    if (ass_oper_ast == NULL) {
        err = ruyi_error_by_parser(reader, "miss assignment operator");
        goto assignment_on_error;
    }
    if ((err = assignment_expression(reader, &ass_expr_ast)) != NULL) {
        goto assignment_on_error;
    }
    if (ass_expr_ast == NULL) {
        err = ruyi_error_by_parser(reader, "miss assignment expression");
        goto assignment_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_assignment);
    ruyi_ast_add_child(ast, left_hand_side_ast);
    ruyi_ast_add_child(ast, ass_oper_ast);
    ruyi_ast_add_child(ast, ass_expr_ast);
    *out_ast = ast;
    return NULL;
assignment_on_error:
    if (left_hand_side_ast != NULL) {
        ruyi_ast_destroy(left_hand_side_ast);
    }
    if (ass_oper_ast != NULL) {
        ruyi_ast_destroy(ass_oper_ast);
    }
    if (ass_expr_ast != NULL) {
        ruyi_ast_destroy(ass_expr_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* assignment_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    //  <assignment expression> ::= <conditional expression> | <assignment>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = conditional_expression(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = assignment(reader, &ast)) != NULL) {
        return err;
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <expression> ::= <assignment expression>
    return assignment_expression(reader, out_ast);
}

static
ruyi_error* variable_declaration_tail(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable declaration tail> ::= (LBRACKET RBRACKET) * IDENTITY (ASSIGN <expression>) ?
    ruyi_error* err;
    ruyi_token token1;
    ruyi_token token2;
    ruyi_ast *var_declare_ast = NULL;
    ruyi_ast *ast_var_init = NULL;
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
    if ((err = expression(reader, &ast_var_init)) != NULL) {
        goto variable_declaration_tail_on_error;
    }
    if (ast_var_init == NULL) {
        err = ruyi_error_make(Ruyi_et_Parser, "miss expression after '='", &token1);
        goto variable_declaration_tail_on_error;
    }
    ruyi_ast_add_child(var_declare_ast, ast_var_init);
    *out_ast = var_declare_ast;
    return NULL;
variable_declaration_tail_on_error:
    if (var_declare_ast != NULL) {
        ruyi_ast_destroy(var_declare_ast);
    }
    if (ast_var_init != NULL) {
        ruyi_ast_destroy(ast_var_init);
    }
    *out_ast = NULL;
    return err;
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
