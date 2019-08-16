//
//  ruyi_vector.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/16.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_vector.h"
#include "ruyi_mem.h"
#include <string.h> // for memcpy

#define VECTOR_DEFAULT_INIT_CAP 10
#define VECTOR_GROUP_RATE 1.5
#define VECTOR_GROUP_VALUE 10


ruyi_vector* ruyi_vector_create(void) {
    return ruyi_vector_create_with_cap(VECTOR_DEFAULT_INIT_CAP);
}

ruyi_vector* ruyi_vector_create_with_cap(UINT32 init_cap) {
    ruyi_vector *vector;
    vector = (ruyi_vector *)ruyi_mem_alloc(sizeof(ruyi_vector));
    vector->cap = init_cap;
    vector->len = 0;
    if (init_cap == 0) {
        vector->value_data = NULL;
    } else {
        vector->value_data = ruyi_mem_alloc(init_cap * sizeof(ruyi_value));
    }
    return vector;
}

void ruyi_vector_destroy(ruyi_vector* vector) {
    assert(vector);
    if (vector->value_data) {
        ruyi_mem_free(vector->value_data);
    }
    ruyi_mem_free(vector);
}

static void ruyi_vector_growup(ruyi_vector* vector) {
    UINT32 new_cap = (UINT32)(vector->cap * VECTOR_GROUP_RATE) + VECTOR_GROUP_VALUE;
    ruyi_value *new_data = ruyi_mem_alloc(new_cap * sizeof(ruyi_value));
    if (vector->value_data) {
        memcpy(new_data, vector->value_data, vector->len * sizeof(ruyi_value));
        ruyi_mem_free(vector->value_data);
    }
    vector->value_data = new_data;
    vector->cap = new_cap;
}

void ruyi_vector_add(ruyi_vector* vector, ruyi_value value) {
    assert(vector);
    while (vector->len >= vector->cap) {
        ruyi_vector_growup(vector);
    }
    vector->value_data[vector->len++] = value;
}

BOOL ruyi_vector_get(ruyi_vector* vector, UINT32 index, ruyi_value *ret_value) {
    assert(vector);
    if (index >= vector->len) {
        return FALSE;
    }
    *ret_value = vector->value_data[index];
    return TRUE;
}

BOOL ruyi_vector_set(ruyi_vector* vector, UINT32 index, ruyi_value value) {
    assert(vector);
    if (index >= vector->len) {
        return FALSE;
    }
    vector->value_data[index] = value;
    return TRUE;
}

UINT32 ruyi_vector_length(ruyi_vector* vector) {
    assert(vector);
    return vector->len;
}


