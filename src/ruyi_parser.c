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

static ruyi_ast* create_ast_string_by_token(ruyi_lexer_reader *reader, ruyi_ast_type type, ruyi_token **out_token) {
    ruyi_ast * ret;
    ruyi_token *token = ruyi_lexer_reader_next_token(reader);
    ret = ruyi_ast_create_with_unicode(type, token->value.str_value);
    if (out_token == NULL) {
        ruyi_lexer_token_destroy(token);
    } else {
        *out_token = token;
    }
    return ret;
}

static ruyi_ast* create_ast_by_consume_token_string(ruyi_lexer_reader *reader, ruyi_ast_type type) {
    return create_ast_string_by_token(reader, type, NULL);
}

static void tokens_push_back_at_front(ruyi_lexer_reader *reader, ruyi_vector *tokens) {
    UINT32 i, len;
    ruyi_value temp;
    ruyi_token *token;
    len = ruyi_vector_length(tokens);
    for (i = len; i > 0; i--) {
        ruyi_vector_get(tokens, i - 1, &temp);
        token = (ruyi_token*)temp.data.ptr;
        ruyi_lexer_reader_push_front(reader, token);
    }
    ruyi_vector_destroy(tokens);
}

static BOOL is_keyword(ruyi_token_type type) {
    return NULL != ruyi_lexer_keywords_get_str(type);
}


static
ruyi_error* assignment_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* field_access_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* array_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* array_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* map_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* reference_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* unary_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* not_plus_minus_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* primitive_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* type(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* unary_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* name(ruyi_lexer_reader *reader, ruyi_ast **out_ast, ruyi_vector* out_tokens);

static
ruyi_error* array_creation(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* map_creation(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* primary_no_new_collection(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* primary(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* block(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* name(ruyi_lexer_reader *reader, ruyi_ast **out_ast, ruyi_vector* out_tokens);

static
ruyi_error* block_statements(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
ruyi_error* statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast);

static
void statement_ends(ruyi_lexer_reader *reader) {
    // <statement ends> ::= SEMICOLON *
    for (;;) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_SEMICOLON, NULL)) {
            break;
        }
    }
}

static
ruyi_error* type_cast(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <type cast> ::= DOT RPARAN <type> RPARAN
    ruyi_error *err;
    ruyi_ast *type_ast = NULL;
    ruyi_token *dot_token = NULL;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_DOT) {
        *out_ast = NULL;
        return NULL;
    }
    dot_token = ruyi_lexer_reader_next_token(reader);
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        ruyi_lexer_reader_push_front(reader, dot_token);
        *out_ast = NULL;
        return NULL;
    }
    ruyi_lexer_token_destroy(dot_token);
    if ((err = type(reader, &type_ast)) != NULL) {
        return err;
    }
    if (type_ast == NULL) {
        err = ruyi_error_by_parser(reader, "type-cast miss type after '('");
        goto type_castion_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "type-cast miss ')' after type");
        goto type_castion_on_error;
    }
    *out_ast = type_ast;
    return NULL;
type_castion_on_error:
    if (type_ast != NULL) {
        ruyi_ast_destroy(type_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* postfix_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <postfix expression> ::= ( <field access expression> | <name> ) (DEC | INC | <type cast>) ?
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *type_cast_ast = NULL;
    if ((err = primary(reader, &ast)) != NULL) {
        *out_ast = NULL;
        return err;
    }
    if (ast == NULL) {
        if ((err = name(reader, &ast, NULL)) != NULL) {
            *out_ast = NULL;
            return err;
        }
        if (ast == NULL) {
            *out_ast = NULL;
            return NULL;
        }
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_DEC, NULL)) {
        *out_ast = ruyi_ast_create(Ruyi_at_postfix_dec_expression);
        ruyi_ast_add_child(*out_ast, ast);
        return NULL;
    } else if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_INC, NULL)) {
        *out_ast = ruyi_ast_create(Ruyi_at_postfix_inc_expression);
        ruyi_ast_add_child(*out_ast, ast);
        return NULL;
    } else {
        if ((err = type_cast(reader, &type_cast_ast)) != NULL) {
            *out_ast = NULL;
            return err;
        }
        if (type_cast_ast == NULL) {
            *out_ast = ast;
            return NULL;
        } else {
            *out_ast = ruyi_ast_create(Ruyi_at_type_cast_expression);
            ruyi_ast_add_child(*out_ast, ast);
            ruyi_ast_add_child(*out_ast, type_cast_ast);
            return NULL;
        }
    }
}

static
ruyi_error* not_plus_minus_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <not plus minus expression> ::= BIT_INVERSE <unary expression> | LOGIC_NOT <unary expression> | <postfix expression>
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *target_ast;
    ruyi_token_type token_type;
    token_type = ruyi_lexer_reader_peek_token_type(reader);
    if  (token_type == Ruyi_tt_BIT_INVERSE) {
        ruyi_lexer_reader_consume_token(reader);
        if ((err = unary_expression(reader, &target_ast)) != NULL) {
            return err;
        }
        if (target_ast == NULL) {
            return ruyi_error_by_parser(reader, "miss expression after '~'");
        }
        ast = ruyi_ast_create(Ruyi_at_bit_inverse_expression);
        ruyi_ast_add_child(ast, target_ast);
        *out_ast = ast;
        return NULL;
    }
    if (token_type == Ruyi_tt_LOGIC_NOT) {
        ruyi_lexer_reader_consume_token(reader);
        if ((err = unary_expression(reader, &target_ast)) != NULL) {
            return err;
        }
        if (target_ast == NULL) {
            return ruyi_error_by_parser(reader, "miss expression after '!'");
        }
        ast = ruyi_ast_create(Ruyi_at_logic_not_expression);
        ruyi_ast_add_child(ast, target_ast);
        *out_ast = ast;
        return NULL;
    }
    return postfix_expression(reader, out_ast);
}

static
ruyi_error* unary_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <unary expression> ::= ADD <unary expression> | SUB <unary expression> | <not plus minus expression>
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *op_ast = NULL;
    ruyi_ast *target_ast = NULL;
    ruyi_token_type token_type;
    token_type = ruyi_lexer_reader_peek_token_type(reader);
    switch (token_type) {
        case Ruyi_tt_ADD:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_add);
            break;
        case Ruyi_tt_SUB:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_sub);
            break;
        default:
            op_ast = NULL;
            break;
    }
    if (op_ast != NULL) {
        if ((err = unary_expression(reader, &target_ast)) != NULL) {
            goto unary_expression_on_error;
        }
        if (target_ast == NULL) {
            err = ruyi_error_by_parser(reader, "miss expression after '+' or '-'");
            goto unary_expression_on_error;
        }
        ast = ruyi_ast_create(Ruyi_at_unary_expression);
        ruyi_ast_add_child(ast, op_ast);
        ruyi_ast_add_child(ast, target_ast);
        *out_ast = ast;
        return NULL;
    }
    return not_plus_minus_expression(reader, out_ast);
unary_expression_on_error:
    if (op_ast != NULL) {
        ruyi_ast_destroy(op_ast);
    }
    if (target_ast != NULL) {
        ruyi_ast_destroy(target_ast);
    }
    return err;
}

static
ruyi_error* multiplicative_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <multiplicative expression> ::= <unary expression> ((MUL | DIV | MOD) <unary expression>)*
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *left_unary_expr_ast = NULL;
    ruyi_ast *right_unary_expr_ast = NULL;
    ruyi_ast *op_ast = NULL;
    ruyi_token_type token_type;
    if ((err = unary_expression(reader, &left_unary_expr_ast)) != NULL) {
        return err;
    }
    if (left_unary_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    while (TRUE) {
        token_type = ruyi_lexer_reader_peek_token_type(reader);
        switch (token_type) {
            case Ruyi_tt_MUL:
                ruyi_lexer_reader_consume_token(reader);
                op_ast = ruyi_ast_create(Ruyi_at_op_mul);
                break;
            case Ruyi_tt_DIV:
                ruyi_lexer_reader_consume_token(reader);
                op_ast = ruyi_ast_create(Ruyi_at_op_div);
                break;
            case Ruyi_tt_MOD:
                ruyi_lexer_reader_consume_token(reader);
                op_ast = ruyi_ast_create(Ruyi_at_op_mod);
                break;
            default:
                op_ast = NULL;
                break;
        }
        if (op_ast == NULL) {
            break;
        }
        if ((err = multiplicative_expression(reader, &right_unary_expr_ast)) != NULL) {
            goto multiplicative_expression_on_error;
        }
        if (right_unary_expr_ast == NULL) {
            err = ruyi_error_by_parser(reader, "need expression after '*', '/' or '%'");
            goto multiplicative_expression_on_error;
        }
        ast = ruyi_ast_create(Ruyi_at_multiplicative_expression);
        ruyi_ast_add_child(ast, left_unary_expr_ast);
        ruyi_ast_add_child(ast, op_ast);
        ruyi_ast_add_child(ast, right_unary_expr_ast);
        left_unary_expr_ast = ast;
    }
    *out_ast = left_unary_expr_ast;
    return NULL;
multiplicative_expression_on_error:
    if (left_unary_expr_ast != NULL) {
        ruyi_ast_destroy(left_unary_expr_ast);
    }
    if (right_unary_expr_ast != NULL) {
        ruyi_ast_destroy(right_unary_expr_ast);
    }
    if (op_ast != NULL) {
        ruyi_ast_destroy(op_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* additive_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <additive expression> ::= <multiplicative expression> ((ADD | SUB) <multiplicative expression>)*
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *left_mul_expr_ast = NULL;
    ruyi_ast *right_mul_expr_ast = NULL;
    ruyi_ast *op_ast = NULL;
    ruyi_token_type token_type;
    if ((err = multiplicative_expression(reader, &left_mul_expr_ast)) != NULL) {
        return err;
    }
    if (left_mul_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    while (TRUE) {
        token_type = ruyi_lexer_reader_peek_token_type(reader);
        switch (token_type) {
            case Ruyi_tt_ADD:
                ruyi_lexer_reader_consume_token(reader);
                op_ast = ruyi_ast_create(Ruyi_at_op_add);
                break;
            case Ruyi_tt_SUB:
                ruyi_lexer_reader_consume_token(reader);
                op_ast = ruyi_ast_create(Ruyi_at_op_sub);
                break;
            default:
                op_ast = NULL;
                break;
        }
        if (op_ast == NULL) {
            break;
        }
        if ((err = multiplicative_expression(reader, &right_mul_expr_ast)) != NULL) {
            goto additive_expression_on_error;
        }
        if (right_mul_expr_ast == NULL) {
            err = ruyi_error_by_parser(reader, "need expression after '+' or '-'");
            goto additive_expression_on_error;
        }
        ast = ruyi_ast_create(Ruyi_at_additive_expression);
        ruyi_ast_add_child(ast, left_mul_expr_ast);
        ruyi_ast_add_child(ast, op_ast);
        ruyi_ast_add_child(ast, right_mul_expr_ast);
        left_mul_expr_ast = ast;
    }
    *out_ast = left_mul_expr_ast;
    return NULL;
additive_expression_on_error:
    if (left_mul_expr_ast != NULL) {
        ruyi_ast_destroy(left_mul_expr_ast);
    }
    if (right_mul_expr_ast != NULL) {
        ruyi_ast_destroy(right_mul_expr_ast);
    }
    if (op_ast != NULL) {
        ruyi_ast_destroy(op_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* shift_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <shift expression> ::= <additive expression> ((SHFT_LEFT | SHFT_RIGHT) <additive expression>) ?
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *add_expr_ast = NULL;
    ruyi_ast *right_add_expr_ast = NULL;
    ruyi_ast *op_ast = NULL;
    ruyi_token_type token_type;
    if ((err = additive_expression(reader, &add_expr_ast)) != NULL) {
        return err;
    }
    if (add_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    token_type = ruyi_lexer_reader_peek_token_type(reader);
    switch (token_type) {
        case Ruyi_tt_SHFT_LEFT:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_shift_left);
            break;
        case Ruyi_tt_SHFT_RIGHT:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_shift_right);
            break;
        default:
            op_ast = NULL;
            break;
    }
    if (op_ast == NULL) {
        *out_ast = add_expr_ast;
        return NULL;
    }
    if ((err = additive_expression(reader, &right_add_expr_ast)) != NULL) {
        goto shift_expression_on_error;
    }
    if (right_add_expr_ast == NULL) {
        err = ruyi_error_by_parser(reader, "need expression after shift operator");
        goto shift_expression_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_shift_expression);
    ruyi_ast_add_child(ast, add_expr_ast);
    ruyi_ast_add_child(ast, op_ast);
    ruyi_ast_add_child(ast, right_add_expr_ast);
    *out_ast = ast;
    return NULL;
shift_expression_on_error:
    if (add_expr_ast != NULL) {
        ruyi_ast_destroy(add_expr_ast);
    }
    if (right_add_expr_ast != NULL) {
        ruyi_ast_destroy(right_add_expr_ast);
    }
    if (op_ast != NULL) {
        ruyi_ast_destroy(op_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* relational_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <relational expression> ::= <shift expression> ((KW_INSTANCEOF <reference type>) | ((LT | GT | LTE | GTE) <shift expression>)) ?
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *shift_expr_ast = NULL;
    ruyi_ast *right_shift_expr_ast = NULL;
    ruyi_ast *ref_type_ast = NULL;
    ruyi_ast *op_ast = NULL;
    ruyi_token_type token_type;
    if ((err = shift_expression(reader, &shift_expr_ast)) != NULL) {
        return err;
    }
    if (shift_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    token_type = ruyi_lexer_reader_peek_token_type(reader);
    if (token_type == Ruyi_tt_KW_INSTANCEOF) {
        ruyi_lexer_reader_consume_token(reader); // consume instanceof
        if ((err = reference_type(reader, &ref_type_ast)) != NULL) {
            goto relational_expression_on_error;
        }
        if (ref_type_ast == NULL) {
            err = ruyi_error_by_parser(reader, "miss type-name after 'instanceof'");
            goto relational_expression_on_error;
        }
        ast = ruyi_ast_create(Ruyi_at_relational_expression);
        ruyi_ast_add_child(ast, shift_expr_ast);
        ruyi_ast_add_child(ast, ruyi_ast_create(Ruyi_at_op_instanceof));
        ruyi_ast_add_child(ast, ref_type_ast);
        *out_ast = ast;
        return NULL;
    }
    switch (token_type) {
        case Ruyi_tt_LT:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_lt);
            break;
        case Ruyi_tt_GT:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_gt);
            break;
        case Ruyi_tt_LTE:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_lte);
            break;
        case Ruyi_tt_GTE:
            ruyi_lexer_reader_consume_token(reader);
            op_ast = ruyi_ast_create(Ruyi_at_op_gte);
            break;
        default:
            op_ast = NULL;
            break;
    }
    if (op_ast == NULL) {
        *out_ast = shift_expr_ast;
        return NULL;
    }
    if ((err = shift_expression(reader, &right_shift_expr_ast)) != NULL) {
        goto relational_expression_on_error;
    }
    if (right_shift_expr_ast == NULL) {
        err = ruyi_error_by_parser(reader, "need expression after compare operator");
        goto relational_expression_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_relational_expression);
    ruyi_ast_add_child(ast, shift_expr_ast);
    ruyi_ast_add_child(ast, op_ast);
    ruyi_ast_add_child(ast, right_shift_expr_ast);
    *out_ast = ast;
    return NULL;
relational_expression_on_error:
    if (shift_expr_ast != NULL) {
        ruyi_ast_destroy(shift_expr_ast);
    }
    if (right_shift_expr_ast != NULL) {
        ruyi_ast_destroy(right_shift_expr_ast);
    }
    if (op_ast != NULL) {
        ruyi_ast_destroy(op_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* equality_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <equality expression> ::= <relational expression> ((EQUALS | NOT_EQUALS) <relational expression>)?
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *rel_expr_ast = NULL;
    ruyi_token_type token_type;
    if ((err = relational_expression(reader, &rel_expr_ast)) != NULL) {
        return err;
    }
    if (rel_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    token_type = ruyi_lexer_reader_peek_token_type(reader);
    if (token_type != Ruyi_tt_EQUALS && token_type != Ruyi_tt_NOT_EQUALS) {
        *out_ast = rel_expr_ast;
        return NULL;
    }
    ruyi_lexer_reader_consume_token(reader); // consume EQUALS or NOT_EQUALS
    ast = ruyi_ast_create(Ruyi_at_equality_expression);
    ruyi_ast_add_child(ast, rel_expr_ast);
    switch (token_type) {
        case Ruyi_tt_EQUALS:
            ruyi_ast_add_child(ast, ruyi_ast_create(Ruyi_at_op_equals));
            break;
        case Ruyi_tt_NOT_EQUALS:
            ruyi_ast_add_child(ast, ruyi_ast_create(Ruyi_at_op_not_equals));
            break;
        default:
            // may not reach here
            break;
    }
    if ((err = relational_expression(reader, &rel_expr_ast)) != NULL) {
        goto equality_expression_on_error;
    }
    if (rel_expr_ast == NULL) {
        if (Ruyi_tt_EQUALS == token_type) {
            err = ruyi_error_by_parser(reader, "miss expression after '=='");
        } else {
            err = ruyi_error_by_parser(reader, "miss expression after '!='");
        }
        goto equality_expression_on_error;
    }
    ruyi_ast_add_child(ast, rel_expr_ast);
    *out_ast = ast;
    return NULL;
equality_expression_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* bit_and_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <bit and expression> ::= <equality expression> (BIT_AND <equality expression>)*
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *eq_expr_ast = NULL;
    if ((err = equality_expression(reader, &eq_expr_ast)) != NULL) {
        return err;
    }
    if (eq_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_BIT_AND) {
        *out_ast = eq_expr_ast;
        return NULL;
    }
    ast = ruyi_ast_create(Ruyi_at_bit_and_expression);
    ruyi_ast_add_child(ast, eq_expr_ast);
    
    while (TRUE) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_BIT_AND, NULL)) {
            break;
        }
        if ((err = equality_expression(reader, &eq_expr_ast)) != NULL) {
            goto bit_and_expression_on_error;
        }
        if (eq_expr_ast == NULL) {
            err = ruyi_error_by_parser(reader, "miss expression after '&'");
            goto bit_and_expression_on_error;
        }
        ruyi_ast_add_child(ast, eq_expr_ast);
    }
    *out_ast = ast;
    return NULL;
bit_and_expression_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* bit_or_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <bit or expression> ::= <bit and expression> (BIT_OR <bit and expression>)*
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *bit_and_expr_ast = NULL;
    if ((err = bit_and_expression(reader, &bit_and_expr_ast)) != NULL) {
        return err;
    }
    if (bit_and_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_BIT_OR) {
        *out_ast = bit_and_expr_ast;
        return NULL;
    }
    ast = ruyi_ast_create(Ruyi_at_bit_or_expression);
    ruyi_ast_add_child(ast, bit_and_expr_ast);
    
    while (TRUE) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_BIT_OR, NULL)) {
            break;
        }
        if ((err = bit_and_expression(reader, &bit_and_expr_ast)) != NULL) {
            goto bit_or_expression_on_error;
        }
        if (bit_and_expr_ast == NULL) {
            err = ruyi_error_by_parser(reader, "miss expression after '|'");
            goto bit_or_expression_on_error;
        }
        ruyi_ast_add_child(ast, bit_and_expr_ast);
    }
    *out_ast = ast;
    return NULL;
bit_or_expression_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* conditional_and_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <conditional and expression> ::= <bit or expression> ( LOGIC_AND <bit or expression>)*
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *bit_or_expr_ast = NULL;
    if ((err = bit_or_expression(reader, &bit_or_expr_ast)) != NULL) {
        return err;
    }
    if (bit_or_expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_LOGIC_AND) {
        *out_ast = bit_or_expr_ast;
        return NULL;
    }
    ast = ruyi_ast_create(Ruyi_at_conditional_and_expression);
    ruyi_ast_add_child(ast, bit_or_expr_ast);
    
    while (TRUE) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LOGIC_AND, NULL)) {
            break;
        }
        if ((err = conditional_and_expression(reader, &bit_or_expr_ast)) != NULL) {
            goto conditional_and_expression_on_error;
        }
        if (bit_or_expr_ast == NULL) {
            err = ruyi_error_by_parser(reader, "miss expression after '&&'");
            goto conditional_and_expression_on_error;
        }
        ruyi_ast_add_child(ast, bit_or_expr_ast);
    }
    *out_ast = ast;
    return NULL;
conditional_and_expression_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* conditional_or_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <conditional or expression> ::= <conditional and expression> (LOGIC_OR <conditional and expression>)*
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *expr_ast = NULL;
    if ((err = conditional_and_expression(reader, &expr_ast)) != NULL) {
        return err;
    }
    if (expr_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_LOGIC_OR) {
        *out_ast = expr_ast;
        return NULL;
    }
    ast = ruyi_ast_create(Ruyi_at_conditional_or_expression);
    ruyi_ast_add_child(ast, expr_ast);
    while (TRUE) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LOGIC_OR, NULL)) {
            break;
        }
        if ((err = conditional_and_expression(reader, &expr_ast)) != NULL) {
            goto conditional_or_expression_on_error;
        }
        if (expr_ast == NULL) {
            err = ruyi_error_by_parser(reader, "miss expression after '||'");
            goto conditional_or_expression_on_error;
        }
        ruyi_ast_add_child(ast, expr_ast);
    }
    *out_ast = ast;
    return NULL;
conditional_or_expression_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* conditional_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <conditional or expression> (QM <expression> Ruyi_tt_COLON <conditional expression>)?
    ruyi_error *err;
    ruyi_ast *conditon_expr_ast = NULL;
    ruyi_ast *true_expr_ast = NULL;
    ruyi_ast *false_expr_ast = NULL;
    ruyi_ast * ast;
    if ((err = conditional_or_expression(reader, &conditon_expr_ast)) != NULL) {
        return err;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_QM, NULL)) {
        *out_ast = conditon_expr_ast;
        return NULL;
    }
    if ((err = conditional_or_expression(reader, &true_expr_ast)) != NULL) {
        goto conditional_expression_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COLON, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ':' in conditional expression");
        goto conditional_expression_on_error;
    }
    if ((err = conditional_expression(reader, &false_expr_ast)) != NULL) {
        goto conditional_expression_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_conditional_expression);
    ruyi_ast_add_child(ast, conditon_expr_ast);
    ruyi_ast_add_child(ast, true_expr_ast);
    ruyi_ast_add_child(ast, false_expr_ast);
    *out_ast = ast;
    return NULL;
conditional_expression_on_error:
    if (conditon_expr_ast != NULL) {
        ruyi_ast_destroy(conditon_expr_ast);
    }
    if (true_expr_ast != NULL) {
        ruyi_ast_destroy(true_expr_ast);
    }
    if (false_expr_ast != NULL) {
        ruyi_ast_destroy(false_expr_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* name(ruyi_lexer_reader *reader, ruyi_ast **out_ast, ruyi_vector* out_tokens) {
    // <name> ::= IDENTITY (DOT IDENTITY) *
    ruyi_error *err;
    ruyi_ast *name = NULL;
    ruyi_token *token;
    ruyi_value temp_value;
    ruyi_vector* out_tokens_temp = NULL;
    UINT32 i, len;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        *out_ast = NULL;
        return NULL;
    }
    if (out_tokens == NULL) {
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
    } else {
        out_tokens_temp = ruyi_vector_create();
        name = create_ast_string_by_token(reader, Ruyi_at_name, &token);
        ruyi_vector_add(out_tokens_temp, ruyi_value_ptr(token));
        while (TRUE) {
            if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_DOT) {
                break;
            }
            token = ruyi_lexer_reader_next_token(reader); // token: DOT
            // add token: DOT
            ruyi_vector_add(out_tokens_temp, ruyi_value_ptr(token));

            if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
                err = ruyi_error_by_parser(reader, "need an identifier after '.'");
                goto name_on_error;
            }
            ruyi_ast_add_child(name, create_ast_string_by_token(reader, Ruyi_at_name_part, &token));
            // add name part
            ruyi_vector_add(out_tokens_temp, ruyi_value_ptr(token));
        }
        *out_ast = name;
        ruyi_vector_add_all(out_tokens, out_tokens_temp);
        out_tokens_temp = NULL;
        return NULL;
    }
name_on_error:
    if (name) {
        ruyi_ast_destroy(name);
    }
    if (out_tokens_temp != NULL) {
        len = ruyi_vector_length(out_tokens_temp);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(out_tokens_temp, i, &temp_value);
            ruyi_lexer_token_destroy((ruyi_token*)temp_value.data.ptr);
        }
        ruyi_vector_destroy(out_tokens_temp);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* literal(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <literal> ::= INTEGER | FLOAT | KW_TRUE | KW_FALSE | RUNE | STRING | KW_NULL | <name>
    ruyi_error *err = NULL;
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
            ruyi_lexer_reader_push_front(reader, token);
            if ((err = name(reader, &ast, NULL)) != NULL) {
                goto literal_on_error;
            }
            *out_ast = ast;
            return NULL;
            /*
            if (ast != NULL) {
                break;
            }
          //  ruyi_lexer_reader_push_front(reader, token);
            *out_ast = NULL;
            return NULL;
             */
    }
    ruyi_lexer_token_destroy(token);
    *out_ast = ast;
    return NULL;
literal_on_error:
    if (token) {
        ruyi_lexer_token_destroy(token);
    }
    return err;
}

static
ruyi_error* integral_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <integral type> ::= KW_BYTE | KW_SHORT| KW_INT | KW_RUNE | KW_LONG
    ruyi_token_type token_type = ruyi_lexer_reader_peek_token_type(reader);
    switch (token_type) {
        case Ruyi_tt_KW_BYTE:
            ruyi_lexer_reader_consume_token(reader);
            *out_ast = ruyi_ast_create(Ruyi_at_type_byte);
            return NULL;
        case Ruyi_tt_KW_SHORT:
            ruyi_lexer_reader_consume_token(reader);
            *out_ast = ruyi_ast_create(Ruyi_at_type_short);
            return NULL;
        case Ruyi_tt_KW_INT:
            ruyi_lexer_reader_consume_token(reader);
            *out_ast = ruyi_ast_create(Ruyi_at_type_int);
            return NULL;
        case Ruyi_tt_KW_RUNE:
            ruyi_lexer_reader_consume_token(reader);
            *out_ast = ruyi_ast_create(Ruyi_at_type_rune);
            return NULL;
        case Ruyi_tt_KW_LONG:
            ruyi_lexer_reader_consume_token(reader);
            *out_ast = ruyi_ast_create(Ruyi_at_type_long);
            return NULL;
        default:
            break;
    }
    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* floating_point_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <floating-point type> ::= KW_FLOAT | KW_DOUBLE
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_FLOAT, NULL)) {
        *out_ast = ruyi_ast_create(Ruyi_at_type_float);
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_DOUBLE, NULL)) {
        *out_ast = ruyi_ast_create(Ruyi_at_type_double);
        return NULL;
    }
    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* numeric_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <numeric type> ::= <integral type> | <floating-point type>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = integral_type(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = floating_point_type(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* primitive_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <primitive type> ::= <numeric type> | bool
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = numeric_type(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_BOOL, NULL)) {
        *out_ast = ruyi_ast_create(Ruyi_at_type_bool);
        return NULL;
    }
    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* reference_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <reference type> ::= IDENTITY | <array type> | <map type>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_IDENTITY, NULL)) {
        ast = create_ast_by_consume_token_string(reader, Ruyi_at_name);
        *out_ast = ast;
        return NULL;
    }
    if ((err = array_type(reader, out_ast)) != NULL) {
        return err;
    }
    if (*out_ast != NULL) {
        return NULL;
    }
    if ((err = map_type(reader, out_ast)) != NULL) {
        return NULL;
    }
    if (*out_ast != NULL) {
        return NULL;
    }
    *out_ast = NULL;
    return NULL;
}


static
ruyi_error* type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <type> ::= <primitive type> | <reference type>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = primitive_type(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = reference_type(reader, &ast)) != NULL) {
        return err;
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* array_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array type> ::= LBRACKET RBRACKET <type>
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_token *lbracket_token;
    ruyi_ast *type_ast = NULL;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_LBRACKET) {
        *out_ast = NULL;
        return NULL;
    }
    lbracket_token = ruyi_lexer_reader_next_token(reader);
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_RBRACKET) {
        ruyi_lexer_reader_push_front(reader, lbracket_token);
        *out_ast = NULL;
        return NULL;
    }
    ruyi_lexer_token_destroy(lbracket_token);
    if ((err = type(reader, &type_ast)) != NULL) {
        return err;
    }
    if (type_ast == NULL) {
        return ruyi_error_by_parser(reader, "miss type after ']'");
    }
    ast = ruyi_ast_create(Ruyi_at_array_type);
    ruyi_ast_add_child(ast, type_ast);
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* map_type(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <map type> ::= LBRACKET IDENTITY RBRACKET <type>
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_token *lbracket_token;
    ruyi_ast *key_ast = NULL;
    ruyi_ast *value_ast = NULL;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_LBRACKET) {
        *out_ast = NULL;
        return NULL;
    }
    lbracket_token = ruyi_lexer_reader_next_token(reader);
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        ruyi_lexer_reader_push_front(reader, lbracket_token);
        *out_ast = NULL;
        return NULL;
    }
    ruyi_lexer_token_destroy(lbracket_token);
    key_ast = create_ast_by_consume_token_string(reader, Ruyi_at_name);
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACKET, NULL)) {
        err = ruyi_error_by_parser(reader, "need ']' after identifier when define map type");
        goto map_type_on_error;
    }
    if ((err = type(reader, &value_ast)) != NULL) {
        goto map_type_on_error;
    }
    if (value_ast == NULL) {
        err = ruyi_error_by_parser(reader, "miss type after ']'");
        goto map_type_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_map_type);
    ruyi_ast_add_child(ast, key_ast);
    ruyi_ast_add_child(ast, value_ast);
    *out_ast = ast;
    return NULL;
map_type_on_error:
    if (key_ast != NULL) {
        ruyi_ast_destroy(key_ast);
    }
    if (value_ast != NULL) {
        ruyi_ast_destroy(value_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* argument_list(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <argument list> ::= <expression> ( COMMA <expression> )*
    ruyi_error *err;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *ast;
    if ((err = expression(reader, &expr_ast)) != NULL) {
        return err;
    }
    ast = ruyi_ast_create(Ruyi_at_argument_list);
    while (expr_ast != NULL) {
        ruyi_ast_add_child(ast, expr_ast);
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
            break;
        }
        if ((err = expression(reader, &expr_ast)) != NULL) {
            goto argument_list_on_error;
        }
    }
    *out_ast = ast;
    return NULL;
argument_list_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* function_invocation_tail(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <function invocation tail> ::= LPARAN <argument list>? RPARAN
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *arg_list_ast = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = argument_list(reader, &arg_list_ast)) != NULL) {
        goto function_invocation_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ')'");
        goto function_invocation_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_function_invocation_tail);
    ruyi_ast_add_child(ast, arg_list_ast);
    *out_ast = ast;
    return NULL;
function_invocation_on_error:
    if (arg_list_ast != NULL) {
        ruyi_ast_destroy(arg_list_ast);
    }
    *out_ast = NULL;
    return err;
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
        ruyi_lexer_reader_push_front(reader, name_token);
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
    // <primary no new collection> ::= <literal> <function invocation tail>? | KW_THIS | LPARAN <expression> RPARAN | <instance creation>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_func_invocation = NULL;
    ruyi_ast *ast_func_invocation_tail = NULL;
    if ((err = literal(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        if ((err = function_invocation_tail(reader, &ast_func_invocation_tail)) != NULL) {
            ruyi_ast_destroy(ast);
            return err;
        }
        // for function call
        if (ast_func_invocation_tail != NULL) {
            ast_func_invocation = ruyi_ast_create(Ruyi_at_function_invocation);
            ruyi_ast_add_child(ast_func_invocation, ast);
            ruyi_ast_add_child(ast_func_invocation, ast_func_invocation_tail);
            *out_ast = ast_func_invocation;
            return NULL;
        }
        // for simple literal
        if (ast != NULL) {
            *out_ast = ast;
            return NULL;
        }
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
        if (ast == NULL) {
            return ruyi_error_by_parser(reader, "need expression in '(' and ')'");
        }
        *out_ast = ast;
        return NULL;
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
ruyi_error* dot_expression_tail(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <dot expression tail> ::= IDENTITY (LPARAN <argument list>? RPARAN)?

    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* bracket_expression_tail(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <bracket expression tail> ::= <expression> RBRACKET

    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* field_access_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <field access expression> ::= <primary> (DOT <dot expression tail> | LBRACKET <bracket expression tail>) *
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *left_ast = NULL;
    ruyi_ast *expr_ast = NULL;
    if ((err = primary(reader, &left_ast)) != NULL) {
        return err;
    }
    if (left_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    while (TRUE) {
        if ((err = dot_expression_tail(reader, &expr_ast)) != NULL) {
            goto field_access_expression_on_error;
        }
        if (expr_ast != NULL) {
            ast = ruyi_ast_create(Ruyi_at_field_dot_access_expression);
            ruyi_ast_add_child(ast, left_ast);
            ruyi_ast_add_child(ast, expr_ast);
            left_ast = ast;
            continue;
        }
        if ((err = bracket_expression_tail(reader, &expr_ast)) != NULL) {
            goto field_access_expression_on_error;
        }
        if (expr_ast != NULL) {
            ast = ruyi_ast_create(Ruyi_at_field_bracket_access_expression);
            ruyi_ast_add_child(ast, left_ast);
            ruyi_ast_add_child(ast, expr_ast);
            left_ast = ast;
            continue;
        }
        break;
    }
    *out_ast = left_ast;
    return NULL;
field_access_expression_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    if (left_ast != NULL) {
        ruyi_ast_destroy(left_ast);
    }
    *out_ast = NULL;
    return err;
}


static
ruyi_error* array_variable_access(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <array variable access> ::= <name> LBRACKET <expression> RBRACKET
    ruyi_error *err;
    ruyi_vector *name_tokens = ruyi_vector_create();
    ruyi_ast *name_ast = NULL;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *ast;
    UINT32 i, len;
    ruyi_value temp;
    if ((err = name(reader, &name_ast, name_tokens)) != NULL) {
        return err;
    }
    if (name_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACKET, NULL)) {
        tokens_push_back_at_front(reader, name_tokens);
        *out_ast = NULL;
        return NULL;
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
    if (name_tokens != NULL) {
        len = ruyi_vector_length(name_tokens);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(name_tokens, i, &temp);
            ruyi_lexer_token_destroy((ruyi_token*)temp.data.ptr);
        }
        ruyi_vector_destroy(name_tokens);
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
    //  <left hand side> ::= <name> | <field access expression> | <array access>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if (is_keyword(ruyi_lexer_reader_peek_token_type(reader))) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = name(reader, &ast, NULL)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = field_access_expression(reader, &ast)) != NULL) {
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
    // <variable declaration tail> ::= IDENTITY <type>? (ASSIGN <expression>) ?
    ruyi_error* err;
    ruyi_token token1;
    ruyi_ast *type_ast = NULL;
    ruyi_ast *var_declare_ast = NULL;
    ruyi_ast *ast_var_init = NULL;
   
    if (Ruyi_tt_IDENTITY != ruyi_lexer_reader_peek_token_type(reader)) {
        *out_ast = NULL;
        return NULL;
    }
    var_declare_ast = create_ast_by_consume_token_string(reader, Ruyi_at_var_declaration);
    
    if ((err = type(reader, &type_ast)) != NULL) {
        goto variable_declaration_tail_on_error;
    }
    
    if (type_ast != NULL) {
        ruyi_ast_add_child(var_declare_ast, type_ast);
    } else {
        ruyi_ast_add_child(var_declare_ast, NULL);
    }
    
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
ruyi_error* variable_auto_infer_type_init(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable auto infer type init> ::= IDENTITY COLON_ASSIGN <expression>
    ruyi_error* err;
    ruyi_ast *ast = NULL;
    ruyi_ast *expr_ast = NULL;
    ruyi_token *id_token;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        *out_ast = NULL;
        return NULL;
    }
    id_token = ruyi_lexer_reader_next_token(reader);
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_COLON_ASSIGN) {
        ruyi_lexer_reader_push_front(reader, id_token);
        *out_ast = NULL;
        return NULL;
    }
    ruyi_lexer_reader_push_front(reader, id_token);
    ast = create_ast_by_consume_token_string(reader, Ruyi_at_var_declaration);
    ruyi_lexer_reader_consume_token(reader); // consume Ruyi_tt_COLON_ASSIGN
    if ((err = expression(reader, &expr_ast)) != NULL) {
        goto variable_auto_infer_type_init_on_error;
    }
    if (expr_ast == NULL) {
        err = ruyi_error_by_parser(reader, "need initialize expression after ':='");
        goto variable_auto_infer_type_init_on_error;
    }
    ruyi_ast_add_child(ast, ruyi_ast_create(Ruyi_at_var_declaration_auto_type));
    ruyi_ast_add_child(ast, expr_ast);
    *out_ast = ast;
    return NULL;
variable_auto_infer_type_init_on_error:
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* variable_declaration(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <variable declaration> ::= <variable auto infer type init> | (KW_VAR <variable declaration tail>)
    ruyi_error* err;
    ruyi_ast *ast;
    ruyi_ast *var_declare_ast;
    if ((err = variable_auto_infer_type_init(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_VAR, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = variable_declaration_tail(reader, &var_declare_ast)) != NULL) {
        return err;
    }
    if (var_declare_ast == NULL) {
        return ruyi_error_by_parser(reader, "miss (array or map) identifier after 'var'");
    }
    *out_ast = var_declare_ast;
    return NULL;
}

static
ruyi_error* formal_parameter(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <formal parameter> ::= IDENTITY DOT3? <type>
    ruyi_error *err;
    ruyi_ast *ast_name = NULL;
    ruyi_ast *ast_type = NULL;
    ruyi_ast *ast_type_temp;
    ruyi_ast *ast;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        *out_ast = NULL;
        return NULL;
    }
    ast_name = create_ast_by_consume_token_string(reader, Ruyi_at_name);
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_DOT3, NULL)) {
        ast_type = ruyi_ast_create(Ruyi_at_var_args_type);
    }
    if ((err = type(reader, &ast_type_temp)) != NULL) {
        goto formal_parameter_on_error;
    }
    if (ast_type_temp == NULL) {
        err = ruyi_error_by_parser(reader, "miss type after identifier");
        goto formal_parameter_on_error;
    }
    if (ast_type == NULL) {
        ast_type = ast_type_temp;
    } else {
        ruyi_ast_add_child(ast_type, ast_type_temp);
    }
    ast = ruyi_ast_create(Ruyi_at_formal_parameter);
    ruyi_ast_add_child(ast, ast_name);
    ruyi_ast_add_child(ast, ast_type);
    *out_ast = ast;
    return NULL;
formal_parameter_on_error:
    if (ast_name) {
        ruyi_ast_destroy(ast_name);
    }
    if (ast_type) {
        ruyi_ast_destroy(ast_type);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* formal_parameter_list(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <formal parameter list> ::= ( <formal parameter> ( COMMA <formal parameter>) * ) ?
    ruyi_error *err;
    ruyi_ast *ast_formal_param = NULL;
    ruyi_ast *ast = ruyi_ast_create(Ruyi_at_formal_parameter_list);
    if ((err = formal_parameter(reader, &ast_formal_param)) != NULL) {
        goto formal_parameter_list_on_error;
    }
    if (ast_formal_param == NULL) {
        // no params
        *out_ast = ast;
        return NULL;
    }
    ruyi_ast_add_child(ast, ast_formal_param);
    for (;;) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
            break;
        }
        if ((err = formal_parameter(reader, &ast_formal_param)) != NULL) {
            goto formal_parameter_list_on_error;
        }
        if (ast_formal_param == NULL) {
            err = ruyi_error_by_parser(reader, "miss formal parameter after ','");
            goto formal_parameter_list_on_error;
        }
        ruyi_ast_add_child(ast, ast_formal_param);
    }
    *out_ast = ast;
    return NULL;
formal_parameter_list_on_error:
    if (ast) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* local_variable_declaration(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <local variable declaration> ::= <variable declaration> | <variable auto infer type init>
    ruyi_error *err;
    ruyi_ast *ast;
    if ((err = variable_declaration(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = variable_auto_infer_type_init(reader, &ast)) != NULL) {
        return err;
    }
    *out_ast = ast;
    return NULL;
}


static
ruyi_error* elseif_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <elseif statement> ::= KW_ELSEIF <expression> <block>
    ruyi_error *err;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *block_ast = NULL;
    ruyi_ast *ast;
    BOOL has_paran = FALSE;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_ELSEIF, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        has_paran = TRUE;
    }
    if ((err = expression(reader, &expr_ast)) != NULL) {
        goto elseif_statement_on_error;
    }
    if (has_paran && !ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss )");
        goto elseif_statement_on_error;
    }
    if ((err = block(reader, &block_ast)) != NULL) {
        goto elseif_statement_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_elseif_statement);
    ruyi_ast_add_child(ast, expr_ast);
    ruyi_ast_add_child(ast, block_ast);
    *out_ast = ast;
    return NULL;
elseif_statement_on_error:
    if (expr_ast) {
        ruyi_ast_destroy(expr_ast);
    }
    if (block_ast) {
        ruyi_ast_destroy(block_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* else_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <else statement> ::= KW_ELSE <block>
    ruyi_error *err;
    ruyi_ast *block_ast = NULL;
    ruyi_ast *ast;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_ELSE, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = block(reader, &block_ast)) != NULL) {
        *out_ast = NULL;
        return err;
    }
    ast = ruyi_ast_create(Ruyi_at_else_statement);
    ruyi_ast_add_child(ast, block_ast);
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* if_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <if statement> ::= KW_IF ( LPARAN <expression> RPARAN ) | <expression> <block> <elseif statement>* <else statement>?
    ruyi_error *err;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *block_ast = NULL;
    ruyi_ast *elseif_ast = NULL;
    ruyi_ast *else_ast = NULL;
    ruyi_ast *ast = NULL;
    BOOL has_paran = FALSE;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_IF, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        has_paran = TRUE;
    }
    if ((err = expression(reader, &expr_ast)) != NULL) {
        goto if_statement_on_error;
    }
    if (has_paran && !ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss )");
        goto if_statement_on_error;
    }
    if ((err = block(reader, &block_ast)) != NULL) {
        goto if_statement_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_if_statement);
    ruyi_ast_add_child(ast, expr_ast);
    ruyi_ast_add_child(ast, block_ast);
    expr_ast = NULL;
    block_ast = NULL;
    for (;;) {
        if ((err = elseif_statement(reader, &elseif_ast)) != NULL) {
            goto if_statement_on_error;
        }
        if (elseif_ast == NULL) {
            break;
        }
        ruyi_ast_add_child(ast, elseif_ast);
    }
    if ((err = else_statement(reader, &else_ast)) != NULL) {
        goto if_statement_on_error;
    }
    if (else_ast != NULL) {
        ruyi_ast_add_child(ast, else_ast);
    }
    *out_ast = ast;
    return NULL;
if_statement_on_error:
    if (expr_ast != NULL) {
        ruyi_ast_destroy(expr_ast);
    }
    if (block_ast != NULL) {
        ruyi_ast_destroy(block_ast);
    }
    if (ast != NULL) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}


static
ruyi_error* return_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <return statement> ::= KW_RETURN <expression>? <statement ends>
    ruyi_error *err;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *ast;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_RETURN, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = expression(reader, &expr_ast)) != NULL) {
        return err;
    }
    statement_ends(reader);
    ast = ruyi_ast_create(Ruyi_at_return_statement);
    if (expr_ast != NULL) {
        ruyi_ast_add_child(ast, expr_ast);
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* while_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <while statement> ::= KW_WHILE <expression> | ( LPARAN <expression> RPARAN ) <block>
    ruyi_error *err;
    ruyi_ast *expr_ast = NULL;
    ruyi_ast *ast;
    ruyi_ast *block_ast;
    BOOL has_paran = FALSE;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_WHILE, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        has_paran = TRUE;
    }
    if ((err = expression(reader, &expr_ast)) != NULL) {
        goto while_statement_on_error;
    }
    if (has_paran && !ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss )");
        goto while_statement_on_error;
    }
    if ((err = block(reader, &block_ast)) != NULL) {
        goto while_statement_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_while_statement);
    ruyi_ast_add_child(ast, expr_ast);
    ruyi_ast_add_child(ast, block_ast);
    *out_ast = ast;
    return NULL;
while_statement_on_error:
    if (expr_ast != NULL) {
        ruyi_ast_destroy(expr_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* instance_creation_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <instance creation expression> ::= <map creation> | <array creation> | <instance creation>
    ruyi_error *err;
    ruyi_ast *ast;
    if ((err = map_creation(reader, &ast)) != NULL) {
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
    
    if ((err = instance_creation(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }    
    return NULL;
}

static
ruyi_error* left_hand_side_expression_tail(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <left hand side expression tail> ::= (ASSIGN <expression>) | (COLON_ASSIGN <expression>) | INC | DEC | <function invocation tail>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *sub_ast;
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_ASSIGN, NULL)) {
        if ((err = expression(reader, &sub_ast)) != NULL) {
            return err;
        }
        if (sub_ast == NULL) {
            return ruyi_error_by_parser(reader, "miss expression after '='");
        }
        ast = ruyi_ast_create(Ruyi_at_assign_statement);
        ruyi_ast_add_child(ast, sub_ast);
        *out_ast = ast;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COLON_ASSIGN, NULL)) {
        if ((err = expression(reader, &sub_ast)) != NULL) {
            return err;
        }
        if (sub_ast == NULL) {
            return ruyi_error_by_parser(reader, "miss expression after ':='");
        }
        ast = ruyi_ast_create(Ruyi_at_var_declaration);
        ruyi_ast_add_child(ast, ruyi_ast_create(Ruyi_at_var_declaration_auto_type));
        ruyi_ast_add_child(ast, sub_ast);
        *out_ast = ast;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_INC, NULL)) {
        ast = ruyi_ast_create(Ruyi_at_inc_statement);
        *out_ast = ast;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_DEC, NULL)) {
        ast = ruyi_ast_create(Ruyi_at_dec_statement);
        *out_ast = ast;
        return NULL;
    }
    if ((err = function_invocation_tail(reader, &sub_ast)) != NULL) {
        return err;
    }
    if (sub_ast != NULL) {
        ast = ruyi_ast_create(Ruyi_at_function_invocation_statement);
        ruyi_ast_add_child(ast, sub_ast);
        *out_ast = ast;
        return NULL;
    }
    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* left_hand_side_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <left hand side expression> ::= <left hand side> <left hand side expression tail>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *left_ast = NULL;
    ruyi_ast *tail_ast = NULL;
    if ((err = left_hand_side(reader, &left_ast)) != NULL) {
        return err;
    }
    if (left_ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = left_hand_side_expression_tail(reader, &tail_ast)) != NULL) {
        goto left_hand_side_expression_on_error;
    }
    if (tail_ast == NULL) {
        err = ruyi_error_by_parser(reader, "only variable access can not be a statement");
        goto left_hand_side_expression_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_left_hand_side_expression);
    ruyi_ast_add_child(ast, left_ast);
    ruyi_ast_add_child(ast, tail_ast);
    *out_ast = ast;
    return NULL;
left_hand_side_expression_on_error:
    if (left_ast) {
        ruyi_ast_destroy(left_ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* statement_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <statement expression> ::= <left hand side expression> | <instance creation expression>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = left_hand_side_expression(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    
    if ((err = instance_creation_expression(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    
    *out_ast = NULL;
    return NULL;
}

static
ruyi_error* expression_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <expression statement> ::= <statement expression> <statement ends>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = statement_expression(reader, &ast)) != NULL) {
        return err;
    }
    if (ast == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    statement_ends(reader);
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* for_init(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <for init> ::= (<variable auto infer type init> (COMMA  <variable auto infer type init>) * ) ?
    ruyi_error *err;
    ruyi_ast *var_init_expr = NULL;
    ruyi_ast *var_init_expr_list = NULL;
    if ((err = variable_auto_infer_type_init(reader, &var_init_expr)) != NULL) {
        *out_ast = NULL;
        return NULL;
    }
    var_init_expr_list = ruyi_ast_create(Ruyi_at_expr_statement_list);
    ruyi_ast_add_child(var_init_expr_list, var_init_expr);
    for (;;) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
            break;
        }
        if ((err = variable_auto_infer_type_init(reader, &var_init_expr)) != NULL) {
            goto for_init_on_error;
        }
        if (var_init_expr == NULL) {
            err = ruyi_error_by_parser(reader, "miss expression statement after ','");
            goto for_init_on_error;
        }
        ruyi_ast_add_child(var_init_expr_list, var_init_expr);
    }
    *out_ast = var_init_expr_list;
    return NULL;
for_init_on_error:
    if (var_init_expr_list) {
        ruyi_ast_destroy(var_init_expr_list);
    }
    return err;
}

static
ruyi_error* for_update(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <for update> ::= (<statement expression> (COMMA <statement expression>) * ) ?
    ruyi_error *err;
    ruyi_ast *stmt_expr = NULL;
    ruyi_ast *stmt_expr_list = NULL;
    if ((err = statement_expression(reader, &stmt_expr)) != NULL) {
        *out_ast = NULL;
        return err;
    }
    stmt_expr_list = ruyi_ast_create(Ruyi_at_stmt_expr_list);
    ruyi_ast_add_child(stmt_expr_list, stmt_expr);
    for (;;) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
            break;
        }
        if ((err = statement_expression(reader, &stmt_expr)) != NULL) {
            goto for_update_on_error;
        }
        if (stmt_expr == NULL) {
            err = ruyi_error_by_parser(reader, "miss expression after ','");
            goto for_update_on_error;
        }
        ruyi_ast_add_child(stmt_expr_list, stmt_expr);
    }
    *out_ast = stmt_expr_list;
    return NULL;
for_update_on_error:
    if (stmt_expr_list) {
        ruyi_ast_destroy(stmt_expr_list);
    }
    return err;
}

static
ruyi_error* for_three_parts(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <for three parts> ::= <for init>? SEMICOLON <expression>? SEMICOLON <for update>?
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_for_init = NULL;
    ruyi_ast *ast_expression = NULL;
    ruyi_ast *ast_for_update = NULL;
    if ((err = for_init(reader, &ast_for_init)) != NULL) {
        goto for_three_parts_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_SEMICOLON, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ';' in for-statement");
        goto for_three_parts_on_error;
    }
    if ((err = expression(reader, &ast_expression)) != NULL) {
        goto for_three_parts_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_SEMICOLON, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ';' in for-statement");
        goto for_three_parts_on_error;
    }
    if ((err = for_update(reader, &ast_for_update)) != NULL) {
        goto for_three_parts_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_for_3_parts_header);
    ruyi_ast_add_child(ast, ast_for_init);
    ruyi_ast_add_child(ast, ast_expression);
    ruyi_ast_add_child(ast, ast_for_update);
    *out_ast = ast;
    return NULL;
for_three_parts_on_error:
    if (ast_for_init) {
        ruyi_ast_destroy(ast_for_init);
    }
    if (ast_expression) {
        ruyi_ast_destroy(ast_expression);
    }
    if (ast_for_update) {
        ruyi_ast_destroy(ast_for_update);
    }
    return err;
}

static
ruyi_error* for_in(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <for in> ::= IDENTITY (COMMA IDENTITY) * KW_IN <expression>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_var_list = NULL;
    ruyi_vector *tokens = NULL;
    ruyi_ast *ast_var_name = NULL;
    ruyi_ast *ast_expr = NULL;
    ruyi_token *token;
    ruyi_value temp;
    UINT32 len, i;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        *out_ast = NULL;
        return NULL;
    }
    tokens = ruyi_vector_create();
    ast_var_list = ruyi_ast_create(Ruyi_at_var_list);
    
    ast_var_name = create_ast_string_by_token(reader, Ruyi_at_name, &token);
    ruyi_ast_add_child(ast_var_list, ast_var_name);
    ruyi_vector_add(tokens, ruyi_value_ptr(token));
    
    for (;;) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
            break;
        }
        if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
            err = ruyi_error_by_parser(reader, "need an identifier after ','");
            goto for_in_on_error;
        }
        ast_var_name = create_ast_string_by_token(reader, Ruyi_at_name, &token);
        ruyi_ast_add_child(ast_var_list, ast_var_name);
        ruyi_vector_add(tokens, ruyi_value_ptr(token));
    }
    
    // if there was not contains 'in' keyword, restore status and return NULL.
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_IN, NULL)) {
        ruyi_ast_destroy(ast_var_list);
        tokens_push_back_at_front(reader, tokens);
        *out_ast = NULL;
        return NULL;
    }
    if ((err = expression(reader, &ast_expr)) != NULL) {
        goto for_in_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_for_in_header);
    ruyi_ast_add_child(ast, ast_var_list);
    ruyi_ast_add_child(ast, ast_expr);
    *out_ast = ast;
    return NULL;
for_in_on_error:
    if (ast_var_list) {
        ruyi_ast_destroy(ast_var_list);
    }
    if (tokens != NULL) {
        len = ruyi_vector_length(tokens);
        for (i = 0; i < len; i++) {
            ruyi_vector_get(tokens, i, &temp);
            ruyi_lexer_token_destroy((ruyi_token*)temp.data.ptr);
        }
        ruyi_vector_destroy(tokens);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* for_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <for statement> ::= KW_FOR (<for in> | <for three parts>) | ( LPARAN <for in> | <for three parts> RPARAN) <block>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_body = NULL;
    ruyi_ast *ast_for_in = NULL;
    ruyi_ast *ast_for_3_parts = NULL;
    BOOL has_paran = FALSE;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_FOR, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        has_paran = TRUE;
    }
    if ((err = for_in(reader, &ast_for_in)) != NULL) {
        goto for_statement_on_error;
    }
    if (ast_for_in == NULL) {
        if ((err = for_three_parts(reader, &ast_for_3_parts)) != NULL) {
            goto for_statement_on_error;
        }
        if (ast_for_3_parts == NULL) {
            err = ruyi_error_by_parser(reader, "for statment syntax error");
            goto for_statement_on_error;
        }
    }
    if (has_paran && !ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss )");
        goto for_statement_on_error;
    }
    if ((err = block(reader, &ast_body)) != NULL) {
        goto for_statement_on_error;
    }
    if (ast_body == NULL) {
        err = ruyi_error_by_parser(reader, "miss for statement body");
        goto for_statement_on_error;
    }
    if (ast_for_in != NULL) {
        ast = ruyi_ast_create(Ruyi_at_for_in_statement);
        ruyi_ast_add_child(ast, ast_for_in);
    } else {
        ast = ruyi_ast_create(Ruyi_at_for_3_parts_statement);
        ruyi_ast_add_child(ast, ast_for_3_parts);
    }
    ruyi_ast_add_child(ast, ast_body);
    *out_ast = ast;
    return NULL;
for_statement_on_error:
    if (ast_body != NULL) {
        ruyi_ast_destroy(ast_body);
    }
    if (ast_for_in != NULL) {
        ruyi_ast_destroy(ast_for_in);
    }
    if (ast_for_3_parts != NULL) {
        ruyi_ast_destroy(ast_for_3_parts);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* switch_default_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <switch default statement> ::= KW_DEFAULT COLON <block statements>
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *ast_block = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_DEFAULT, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COLON, NULL)) {
        return ruyi_error_by_parser(reader, "miss ':' after 'default'");
    }
    if ((err = block_statements(reader, &ast_block)) != NULL) {
        return err;
    }
    // ast_block may be NULL
    ast = ruyi_ast_create(Ruyi_at_switch_default_case_statement);
    ruyi_ast_add_child(ast, ast_block);
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* constant_expression(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <constant expression> ::= <expression>
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *ast_expr;
    if ((err = expression(reader, &ast_expr)) != NULL) {
        return err;
    }
    if (ast_expr == NULL) {
        *out_ast = NULL;
        return NULL;
    }
    ast = ruyi_ast_create(Ruyi_at_constant_expression);
    ruyi_ast_add_child(ast, ast_expr);
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* switch_case_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <switch case statement> ::= KW_CASE <constant expression> (COMMA <constant expression>) * COLON <block statements>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_const_stmt_list = NULL;
    ruyi_ast *ast_const_expr = NULL;
    ruyi_ast *ast_block = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_CASE, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = constant_expression(reader, &ast_const_expr)) != NULL) {
        goto switch_case_statement_on_error;
    }
    if (ast_const_expr == NULL) {
        err = ruyi_error_by_parser(reader, "miss constant value in case-statement");
        goto switch_case_statement_on_error;
    }
    ast_const_stmt_list = ruyi_ast_create(Ruyi_at_const_list);
    ruyi_ast_add_child(ast_const_stmt_list, ast_const_expr);
    for (;;) {
        if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COMMA, NULL)) {
            break;
        }
        if ((err = constant_expression(reader, &ast_const_expr)) != NULL) {
            goto switch_case_statement_on_error;
        }
        if (ast_const_expr == NULL) {
            err = ruyi_error_by_parser(reader, "miss constant value after ',' in case-statement");
            goto switch_case_statement_on_error;
        }
        ruyi_ast_add_child(ast_const_stmt_list, ast_const_expr);
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_COLON, NULL)) {
        return ruyi_error_by_parser(reader, "miss ':' in case-statement");
    }
    if ((err = block_statements(reader, &ast_block)) != NULL) {
        goto switch_case_statement_on_error;
    }
    // ast_block can be NULL
    ast = ruyi_ast_create(Ruyi_at_switch_case_statement);
    ruyi_ast_add_child(ast, ast_const_stmt_list);
    ruyi_ast_add_child(ast, ast_block);
    *out_ast = ast;
    return NULL;
switch_case_statement_on_error:
    if (ast_const_stmt_list) {
        ruyi_ast_destroy(ast_const_stmt_list);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* switch_statement_body(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <switch statement body> ::= LBRACE <switch case statement>* <switch default statement>? RBRACE
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *ast_case_stmt_list = NULL;
    ruyi_ast *ast_default_case_stmt = NULL;
    ruyi_ast *ast_case_stmt = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACE, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '{' in switch-statement");
        goto switch_statement_body_on_error;
    }
    ast_case_stmt_list = ruyi_ast_create(Ruyi_at_switch_case_statement_list);
    for (;;) {
        if ((err = switch_case_statement(reader, &ast_case_stmt)) != NULL) {
            goto switch_statement_body_on_error;
        }
        if (ast_case_stmt == NULL) {
            break;
        }
        ruyi_ast_add_child(ast_case_stmt_list, ast_case_stmt);
    }
    if ((err = switch_default_statement(reader, &ast_default_case_stmt)) != NULL) {
        goto switch_statement_body_on_error;
    }
    // ast_default_case_stmt can be NULL
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACE, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '}' in switch-statement");
        goto switch_statement_body_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_switch_statement_body);
    ruyi_ast_add_child(ast, ast_case_stmt_list);
    ruyi_ast_add_child(ast, ast_default_case_stmt);
    *out_ast = ast;
    return NULL;
switch_statement_body_on_error:
    if (ast_case_stmt_list) {
        ruyi_ast_destroy(ast_case_stmt_list);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* switch_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <switch statement> ::= KW_SWITCH <expression> | ( LPARAN <expression> RPARAN ) <switch statement body>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_expr = NULL;
    ruyi_ast *ast_body = NULL;
    BOOL has_paran = FALSE;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_SWITCH, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        has_paran = TRUE;
    }
    if ((err = expression(reader, &ast_expr)) != NULL) {
        goto switch_statement_on_error;
    }
    if (has_paran && !ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss )");
        goto switch_statement_on_error;
    }
    if ((err = switch_statement_body(reader, &ast_body)) != NULL) {
        goto switch_statement_on_error;
    }
    if (ast_body == NULL) {
        err = ruyi_error_by_parser(reader, "miss body for switch-statement");
        goto switch_statement_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_switch_statement);
    ruyi_ast_add_child(ast, ast_expr);
    ruyi_ast_add_child(ast, ast_body);
    *out_ast = ast;
    return NULL;
switch_statement_on_error:
    if (ast_expr) {
        ruyi_ast_destroy(ast_expr);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* break_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <break statement> ::= KW_BREAK IDENTITY?
    ruyi_ast *ast;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_BREAK, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) == Ruyi_tt_IDENTITY) {
        ast = create_ast_by_consume_token_string(reader, Ruyi_at_break_statement);
    } else {
        ast = ruyi_ast_create(Ruyi_at_break_statement);
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* continue_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <continue statement> ::= KW_CONTINUE IDENTITY?
    ruyi_ast *ast;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_CONTINUE, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) == Ruyi_tt_IDENTITY) {
        ast = create_ast_by_consume_token_string(reader, Ruyi_at_continue_statement);
    } else {
        ast = ruyi_ast_create(Ruyi_at_continue_statement);
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* labeled_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <labeled statement> ::= IDENTITY COLON <statement>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *sub_ast = NULL;
    ruyi_token *label_name = NULL;
    ruyi_token *colon = NULL;
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        *out_ast = NULL;
        return NULL;
    }
    label_name = ruyi_lexer_reader_next_token(reader);
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_COLON) {
        ruyi_lexer_reader_push_front(reader, label_name);
        *out_ast = NULL;
        return NULL;
    }
    colon = ruyi_lexer_reader_next_token(reader);
    if ((err = statement(reader, &sub_ast)) != NULL) {
        goto labeled_statement_on_error;
    }
    if (sub_ast == NULL) {
        err = ruyi_error_by_parser(reader, "miss statement after label");
        goto labeled_statement_on_error;
    }
    ast = ruyi_ast_create_with_unicode(Ruyi_at_labeled_statement, label_name->value.str_value);
    ruyi_ast_add_child(ast, sub_ast);
    
    ruyi_lexer_token_destroy(colon);
    ruyi_lexer_token_destroy(label_name);
  
    *out_ast = ast;
    return NULL;
labeled_statement_on_error:
    if (colon) {
        ruyi_lexer_token_destroy(colon);
    }
    if (label_name) {
        ruyi_lexer_token_destroy(label_name);
    }
    *out_ast = NULL;
    return err;
}


static
ruyi_error* sub_block_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <sub block statement> ::= LBRACE <block statements> RBRACE
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_sub_block = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACE, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = block_statements(reader, &ast_sub_block)) != NULL) {
        return err;
    }
    // ast_sub_block can be NULL
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACE, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '}' at end of block");
        goto sub_block_statement_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_sub_block_statement);
    ruyi_ast_add_child(ast, ast_sub_block);
    *out_ast = ast;
    return NULL;
sub_block_statement_on_error:
    if (ast_sub_block) {
        ruyi_ast_destroy(ast_sub_block);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <statement> ::= <labeled statement> | <if statement> | <while statement> | <expression statement> | <for statement> | <switch statement> | <try statement> | <return statement> | <break statement> | <continue statement> | <sub block statement>
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if ((err = labeled_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = if_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = while_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = expression_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = for_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = switch_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    // TODO <try statement> not implements in this version
    if ((err = return_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = break_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = continue_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = sub_block_statement(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    
    return NULL;
}

static
ruyi_error* local_variable_declaration_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <local variable declaration statement> ::= <local variable declaration> <statement ends>
    ruyi_error *err;
    ruyi_ast *ast;
    if ((err = local_variable_declaration(reader, &ast)) != NULL) {
        *out_ast = NULL;
        return err;
    }
    statement_ends(reader);
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* block_statement(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <block statement> ::= <local variable declaration statement> | <statement>
    ruyi_error *err;
    ruyi_ast *ast;
    if ((err = local_variable_declaration_statement(reader, &ast)) != NULL) {
        *out_ast = NULL;
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    if ((err = statement(reader, &ast)) != NULL) {
        *out_ast = NULL;
        return err;
    }
    *out_ast = ast;
    return NULL;
}

static
ruyi_error* block_statements(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <block statements> ::= <block statement> *
    ruyi_error *err;
    ruyi_ast *ast = ruyi_ast_create(Ruyi_at_block_statements);
    ruyi_ast *ast_block_stmt;
    for (;;) {
        if ((err = block_statement(reader, &ast_block_stmt)) != NULL) {
            goto block_statements_on_error;
        }
        if (ast_block_stmt == NULL) {
            break;
        }
        ruyi_ast_add_child(ast, ast_block_stmt);
    }
    *out_ast = ast;
    return NULL;
block_statements_on_error:
    if (ast) {
        ruyi_ast_destroy(ast);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* block(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <block> ::= LBRACE <block statements>? RBRACE
    ruyi_error *err;
    ruyi_ast *ast = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LBRACE, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if ((err = block_statements(reader, &ast)) != NULL) {
        goto block_on_error;
    }
    if (ast == NULL) {
        err = ruyi_error_by_parser(reader, "block can not be empty");
        goto block_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RBRACE, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '}'");
        goto block_on_error;
    }
    *out_ast = ast;
    return NULL;
block_on_error:
    if (ast) {
        ruyi_ast_destroy(ast);
    }
    return err;
}

static
ruyi_error* function_body(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <function body> ::= <block>
    return block(reader, out_ast);
}

static
ruyi_error* function_declaration(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <function declaration> ::= KW_FUNC IDENTITY LPARAN <formal parameter list>? RPARAN <type>? <function body>
    ruyi_error *err;
    ruyi_ast *ast;
    ruyi_ast *ast_name = NULL;
    ruyi_ast *ast_formal_params = NULL;
    ruyi_ast *ast_return_type = NULL;
    ruyi_ast *ast_body = NULL;
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_KW_FUNC, NULL)) {
        *out_ast = NULL;
        return NULL;
    }
    if (ruyi_lexer_reader_peek_token_type(reader) != Ruyi_tt_IDENTITY) {
        err = ruyi_error_by_parser(reader, "miss identifier after 'func'");
        goto function_declaration_on_error;
    }
    ast_name = create_ast_string_by_token(reader, Ruyi_at_name, NULL);
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_LPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss '(' after identifier when define a function");
        goto function_declaration_on_error;
    }
    if ((err = formal_parameter_list(reader, &ast_formal_params)) != NULL) {
        goto function_declaration_on_error;
    }
    if (!ruyi_lexer_reader_consume_token_if_match(reader, Ruyi_tt_RPAREN, NULL)) {
        err = ruyi_error_by_parser(reader, "miss ')' when define a function");
        goto function_declaration_on_error;
    }
    if ((err = type(reader, &ast_return_type)) != NULL) {
        goto function_declaration_on_error;
    }
    if ((err = function_body(reader, &ast_body)) != NULL) {
        goto function_declaration_on_error;
    }
    if (ast_body == NULL) {
        err = ruyi_error_by_parser(reader, "miss function body");
        goto function_declaration_on_error;
    }
    ast = ruyi_ast_create(Ruyi_at_function_declaration);
    ruyi_ast_add_child(ast, ast_name);
    ruyi_ast_add_child(ast, ast_formal_params);
    ruyi_ast_add_child(ast, ast_return_type);
    ruyi_ast_add_child(ast, ast_body);
    *out_ast = ast;
    return NULL;
function_declaration_on_error:
    if (ast_name) {
        ruyi_ast_destroy(ast_name);
    }
    if (ast_formal_params) {
        ruyi_ast_destroy(ast_formal_params);
    }
    if (ast_return_type) {
        ruyi_ast_destroy(ast_return_type);
    }
    if (ast_body) {
        ruyi_ast_destroy(ast_body);
    }
    *out_ast = NULL;
    return err;
}

static
ruyi_error* global_declaration(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <global declaration> ::= <variable declaration> | <function declaration> | <class declaration> | <interface declaration> | <constant declaration> | <statement ends>
    ruyi_ast *ast = NULL;
    ruyi_error* err;
    if ((err = variable_declaration(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    
    if ((err = function_declaration(reader, &ast)) != NULL) {
        return err;
    }
    if (ast != NULL) {
        *out_ast = ast;
        return NULL;
    }
    // TODO
    
    *out_ast = NULL;
    return NULL;
  //  return ruyi_error_by_parser(reader, "need a variable/function/class/interface/constant declaration.");
}

static
ruyi_error* global_declarations(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <global declarations> ::= <global declaration> *
    ruyi_ast *root;
    ruyi_ast *global_declare_ast = NULL;
    ruyi_error* err;
    root = ruyi_ast_create(Ruyi_at_root);
    while (TRUE) {
        if ((err = global_declaration(reader, &global_declare_ast)) != NULL) {
            return err;
        }
        if (global_declare_ast != NULL) {
            ruyi_ast_add_child(root, global_declare_ast);
        } else {
            // End Of Next global_declaration
            break;
        }
    }
    *out_ast = root;
    return NULL;
}

static
ruyi_error* compilation_unit(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    // <root> ::= <global declarations>?
    return global_declarations(reader, out_ast);
}

ruyi_error* ruyi_parse_ast(ruyi_lexer_reader *reader, ruyi_ast **out_ast) {
    return compilation_unit(reader, out_ast);
}
