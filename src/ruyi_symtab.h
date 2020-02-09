//
//  ruyi_symtab.h
//  ruyi
//
//  Created by Songli Huang on 2019/10/31.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_symtab_h
#define ruyi_symtab_h

#include "ruyi_list.h"
#include "ruyi_vector.h"
#include "ruyi_hashtable.h"
#include "ruyi_unicode.h"
#include "ruyi_error.h"
#include "ruyi_ir.h"

typedef enum {
    Ruyi_sst_Local,
    Ruyi_sst_Global,
    Ruyi_sst_Member
} ruyi_symtab_scope_type;


typedef struct {
    ruyi_list *block_scope_stack;   // item type: ruyi_symtab_index_hashtable.
} ruyi_function_scope;


/*
 Ruyi_ir_type_Object,    // Oname;
 Ruyi_ir_type_Array,     // Atype
 Ruyi_ir_type_Tuple,     // Ttype*
 Ruyi_ir_type_Map,       // Mtypetype
 Ruyi_ir_type_Function,  // F(type*)type*
 */

struct ruyi_symtab_type_object_;
struct ruyi_symtab_type_array_;
struct ruyi_symtab_type_tuple_;
struct ruyi_symtab_type_map_;
struct ruyi_symtab_type_func_;

typedef struct {
    ruyi_ir_type ir_type;
    union {
        void *uniptr;
        struct ruyi_symtab_type_object_  *object;
        struct ruyi_symtab_type_array_   *array;
        struct ruyi_symtab_type_tuple_   *tuple;
        struct ruyi_symtab_type_map_     *map;
        struct ruyi_symtab_type_func_    *func;
    } detail;
} ruyi_symtab_type;

typedef struct ruyi_symtab_type_object_ {
    ruyi_ir_type        type;
    ruyi_unicode_string *name;
} ruyi_symtab_type_object;

typedef struct ruyi_symtab_type_array_ {
    UINT16              dims;
    ruyi_symtab_type    type;
} ruyi_symtab_type_array;

typedef struct ruyi_symtab_type_tuple_ {
    UINT32              count;
    ruyi_symtab_type    *types;
} ruyi_symtab_type_tuple;

typedef struct ruyi_symtab_type_map_ {
    ruyi_symtab_type    key;
    ruyi_symtab_type    value;
} ruyi_symtab_type_map;

typedef struct ruyi_symtab_type_func_ {
    ruyi_symtab_type_tuple  *return_types;
    ruyi_symtab_type_tuple  *parameter_types;
} ruyi_symtab_type_func;


typedef struct {
    ruyi_hashtable  *name2index;
    ruyi_vector     *index2var;
} ruyi_symtab_index_hashtable;

typedef struct {
    UINT32                  index;
    UINT32                  var_size;
    ruyi_symtab_type        type;
    ruyi_symtab_scope_type  scope_type;
    const ruyi_unicode_string *name;
} ruyi_symtab_variable;


typedef struct {
    ruyi_unicode_string *name;
    ruyi_symtab_type    type;
} ruyi_symtab_name_and_type;

typedef struct {
    UINT32          index;
    ruyi_ir_type    type;
    union {
        INT64 int64_value;
        double float64_value;
        ruyi_unicode_string *uncode_str;
    } data;
} ruyi_symtab_constant;

ruyi_symtab_constant *ruyi_symtab_constant_int64(UINT32 index, INT64 value);
ruyi_symtab_constant *ruyi_symtab_constant_float64(UINT32 index, FLOAT64 value);
ruyi_symtab_constant *ruyi_symtab_constant_unicode(UINT32 index, const ruyi_unicode_string* value);
void ruyi_symtab_constant_destroy(ruyi_symtab_constant *c);


typedef struct {
    ruyi_hashtable  *int642index;
    ruyi_hashtable  *float642index;
    ruyi_hashtable  *unicode2index;
    ruyi_vector     *index2value;
} ruyi_symtab_constants_pool;

ruyi_symtab_constants_pool * ruyi_symtab_constants_pool_create(void);
void ruyi_symtab_constants_pool_destroy(ruyi_symtab_constants_pool* constants_pool);
UINT32 ruyi_symtab_constants_pool_get_or_add_int64(ruyi_symtab_constants_pool *cp, INT64 value);
UINT32 ruyi_symtab_constants_pool_get_or_add_float64(ruyi_symtab_constants_pool *cp, FLOAT64 value);
UINT32 ruyi_symtab_constants_pool_get_or_add_unicode(ruyi_symtab_constants_pool *cp, const ruyi_unicode_string *value);



// symbol table:
// global_variables: ==> global variables table (scope):
// functions: ==> name -> function.
// constants: ==> value: integer, float, strings.
// class (global): todo
// interface (global): todo
typedef struct {
    ruyi_symtab_index_hashtable *global_variables;
    ruyi_hashtable *functions;
    ruyi_hashtable *constants;
} ruyi_symtab;

typedef struct {
    UINT32              index;
    BOOL                anonymous;
    const ruyi_unicode_string *name;
    ruyi_symtab         *symtab;        // reference of global ruyi_symtab
    ruyi_function_scope *func_symtab_scope;
    ruyi_vector         *args_types; /* type of ruyi_symtab_name_and_type: (ruyi_unicode_string*, ruyi_ir_type, void*) */
    ruyi_vector         *return_types; /* type of ruyi_symtab_type: (ruyi_ir_type, void*) */
    ruyi_vector         *codes;
    // attributes:
    // UINT16          operand;
    // UINT16          local_size;
    // UINT16          argument_size;
} ruyi_symtab_function_define;


// ================================================================

ruyi_symtab* ruyi_symtab_create(void);

void ruyi_symtab_destroy(ruyi_symtab *symtab);

ruyi_error* ruyi_symtab_add_global_var(ruyi_symtab *symtab, const ruyi_symtab_variable *var, UINT32 *out_index);

BOOL ruyi_symtab_get_global_var_by_name(const ruyi_symtab *symtab, const ruyi_unicode_string *name, ruyi_symtab_variable *out_var);

UINT32 ruyi_symtab_add_constant_int64(ruyi_symtab *symtab, INT64 value, UINT32 *out_index);

UINT32 ruyi_symtab_add_constant_float64(ruyi_symtab *symtab, FLOAT64 value, UINT32 *out_index);

UINT32 ruyi_symtab_add_constant_unicode(ruyi_symtab *symtab, const ruyi_unicode_string *value, UINT32 *out_index);



// ================================================================

ruyi_symtab_function_define* ruyi_symtab_function_create(ruyi_symtab *symtab, const ruyi_unicode_string *name);

void ruyi_symtab_function_destroy(ruyi_symtab_function_define* func);

ruyi_error* ruyi_symtab_function_add_arg(ruyi_symtab_function_define* func, const ruyi_symtab_variable *var);

void ruyi_symtab_function_add_return_type(ruyi_symtab_function_define* func, ruyi_symtab_type return_type);

// ================================================================

ruyi_symtab_type ruyi_symtab_type_create(ruyi_ir_type ir_type, void *detail);

void ruyi_symtab_type_destroy(ruyi_symtab_type type);

ruyi_symtab_type_object* ruyi_symtab_type_object_create(const ruyi_unicode_string *name);

void ruyi_symtab_type_object_destroy(ruyi_symtab_type_object *object);

ruyi_symtab_type_array* ruyi_symtab_type_array_create(UINT16 dims, ruyi_symtab_type type);

void ruyi_symtab_type_array_destroy(ruyi_symtab_type_array *array);

ruyi_symtab_type_tuple* ruyi_symtab_type_tuple_create(UINT32 count);

void ruyi_symtab_type_tuple_set(ruyi_symtab_type_tuple *tuple, UINT32 pos, ruyi_symtab_type type);

void ruyi_symtab_type_tuple_destroy(ruyi_symtab_type_tuple *tuple);

ruyi_symtab_type_map* ruyi_symtab_type_map_create(ruyi_symtab_type key_type, ruyi_symtab_type value_type);

void ruyi_symtab_type_map_destroy(ruyi_symtab_type_map *map);

ruyi_symtab_type_func* ruyi_symtab_type_func_create(ruyi_symtab_type_tuple *return_types, ruyi_symtab_type_tuple  *parameter_types);

void ruyi_symtab_type_func_destroy(ruyi_symtab_type_func *func);

// ====================================================

ruyi_function_scope* ruyi_symtab_function_scope_create(void);

void ruyi_symtab_function_scope_destroy(ruyi_function_scope *function_scope);

void ruyi_symtab_function_scope_enter(ruyi_function_scope* scope);

void ruyi_symtab_function_scope_leave(ruyi_function_scope* scope);

ruyi_error* ruyi_symtab_function_scope_add_var(ruyi_function_scope* scope, const ruyi_symtab_variable *var, UINT32 *out_index);

BOOL ruyi_symtab_function_scope_get(ruyi_function_scope* scope, const ruyi_unicode_string *name, ruyi_symtab_variable *out_var);

// ====================================================


#endif /* ruyi_symtab_h */
