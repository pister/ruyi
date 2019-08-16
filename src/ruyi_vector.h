//
//  ruyi_vector.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/16.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_vector_h
#define ruyi_vector_h
#include "ruyi_value.h"

typedef struct {
    ruyi_value *value_data;
    UINT32 len;
    UINT32 cap;
} ruyi_vector;

/**
 * Create a vector with the default init capacity
 */
ruyi_vector* ruyi_vector_create(void);

/**
 * Create a vector with the init capacity
 * params:
 * init_cap - the init capacity
 */
ruyi_vector* ruyi_vector_create_with_cap(UINT32 init_cap);

/**
 * Free the vector.
 * NOTICE: this will NOT free the item when it is an pointer,
 * so you may free these pointers before ruyi_vector_destroy as youself knows detail.
 * params:
 * vector - the target vector
 */
void ruyi_vector_destroy(ruyi_vector* vector);

/**
 * Add the value at the last of vector
 * params:
 * vector - the target vector
 * value - the value to be add
 */
void ruyi_vector_add(ruyi_vector* vector, ruyi_value value);

/**
 * Get the value at the index of vector
 * params:
 * vector - the target vector
 * index - the item index
 * ret_value - the value to be return
 * return:
 * FALSE indicates out of range of vector, TRUE indicates get value success.
 */
BOOL ruyi_vector_get(ruyi_vector* vector, UINT32 index, ruyi_value *ret_value);

/**
 * Set the value at the index of vector
 * params:
 * vector - the target vector
 * index - the item index
 * value - the value to be set
 * return:
 * FALSE indicates out of range of vector, TRUE indicates get value success.
 */
BOOL ruyi_vector_set(ruyi_vector* vector, UINT32 index, ruyi_value value);

/**
 * Get the length of vector
 * params:
 * vector - the target vector
 * return:
 * the vector length
 */
UINT32 ruyi_vector_length(ruyi_vector* vector);

/**
 * Get the first value index
 * params:
 * vector - the target vector
 * value - the value to be find
 * return:
 * 0 - base index will be return, -1 indicate not found
 */
INT32 ruyi_vector_find_first(ruyi_vector* vector, ruyi_value value);

/**
 * Get the last value index
 * params:
 * vector - the target vector
 * value - the value to be find
 * return:
 * 0 - base index will be return, -1 indicate not found
 */
INT32 ruyi_vector_find_last(ruyi_vector* vector, ruyi_value value);

/**
 * Remove the value at the last of vector
 * params:
 * vector - the target vector
 * ret_last_value - the value has remove, it can be NULL.
 * return:
 * TRUE indicates remove success, or FALSE indicates remove fail (for exmaple: vector is empty.)
 */
BOOL ruyi_vector_remove_last(ruyi_vector* vector, ruyi_value* ret_last_value);

/**
 * Sort the vector
 * params:
 * vector - the target vector
 * comparator - how to compare each items
 */
void ruyi_vector_sort(ruyi_vector* vector, ruyi_value_comparator comparator);

#endif /* ruyi_vector_h */
