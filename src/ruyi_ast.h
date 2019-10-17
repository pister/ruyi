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
#include "ruyi_lexer.h"

typedef enum {
    Ruyi_at_root,
    Ruyi_at_global_declaration,
    Ruyi_at_var_declaration,
    Ruyi_at_var_declaration_auto_type,
    Ruyi_at_assignment,
    Ruyi_at_assign_operator,
    Ruyi_at_name,
    Ruyi_at_name_part,
    Ruyi_at_integer,
    Ruyi_at_float,
    Ruyi_at_bool,
    Ruyi_at_null,
    Ruyi_at_rune,
    Ruyi_at_string,
    Ruyi_at_field_access,
    Ruyi_at_array_variable_access,
    Ruyi_at_array_primary_access,
    Ruyi_at_array_creation_with_init,
    Ruyi_at_array_creation_with_cap,
    Ruyi_at_map_creation,
    Ruyi_at_this,
    Ruyi_at_property,
    Ruyi_at_instance_creation,
    Ruyi_at_array_type,
    Ruyi_at_map_type,
    Ruyi_at_type_bool,
    Ruyi_at_type_byte,
    Ruyi_at_type_short,
    Ruyi_at_type_int,
    Ruyi_at_type_long,
    Ruyi_at_type_rune,
    Ruyi_at_type_float,
    Ruyi_at_type_double,
    Ruyi_at_function_invocation,
    Ruyi_at_function_invocation_tail,
    Ruyi_at_argument_list,
    Ruyi_at_conditional_expression,
    Ruyi_at_conditional_or_expression,
    Ruyi_at_conditional_and_expression,
    Ruyi_at_bit_or_expression,
    Ruyi_at_bit_and_expression,
    Ruyi_at_equality_expression,
    Ruyi_at_op_equals,
    Ruyi_at_op_not_equals,
    Ruyi_at_relational_expression,
    Ruyi_at_op_instanceof,
    Ruyi_at_op_lt,
    Ruyi_at_op_gt,
    Ruyi_at_op_lte,
    Ruyi_at_op_gte,
    Ruyi_at_shift_expression,
    Ruyi_at_op_shift_left,
    Ruyi_at_op_shift_right,
    Ruyi_at_additive_expression,
    Ruyi_at_op_add,
    Ruyi_at_op_sub,
    Ruyi_at_multiplicative_expression,
    Ruyi_at_op_mul,
    Ruyi_at_op_div,
    Ruyi_at_op_mod,
    Ruyi_at_unary_expression,
    Ruyi_at_bit_inverse_expression,
    Ruyi_at_logic_not_expression,
    Ruyi_at_primary_cast_expression,
    Ruyi_at_postfix_dec_expression,
    Ruyi_at_postfix_inc_expression,
    Ruyi_at_type_cast_expression,
    Ruyi_at_field_dot_access_expression,
    Ruyi_at_field_bracket_access_expression

} ruyi_ast_type;


typedef enum {
    Ruyi_adt_value,
    Ruyi_adt_unicode_str,
    Ruyi_adt_char_ptr
} ruyi_ast_data_type;

struct _ruyi_ast;
typedef struct _ruyi_ast {
    ruyi_ast_type type;
    ruyi_ast_data_type adt_type;
    union {
        WIDE_CHAR char_value;
        UINT32 int32_value;
        UINT64 int64_value;
        void* ptr_value;
        double float_value;
    } data;
    ruyi_vector* child_asts;
} ruyi_ast;

ruyi_ast * ruyi_ast_create(ruyi_ast_type type);

ruyi_ast * ruyi_ast_create_by_token_type(ruyi_ast_type type, ruyi_token_type token_type);

ruyi_ast * ruyi_ast_create_with_unicode(ruyi_ast_type type, ruyi_unicode_string *str);

void ruyi_ast_add_child(ruyi_ast *ast, ruyi_ast *child);

UINT32 ruyi_ast_child_length(ruyi_ast *ast);

ruyi_ast * ruyi_ast_get_child(ruyi_ast *ast, UINT32 index);

void ruyi_ast_destroy(ruyi_ast *ast);

#endif /* ruyi_ast_h */
