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
#include <stdlib.h> // for qsort

#define VECTOR_DEFAULT_INIT_CAP 10
#define VECTOR_GROWUP_RATE 1.5
#define VECTOR_GROWUP_VALUE 10


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
    UINT32 new_cap = (UINT32)(vector->cap * VECTOR_GROWUP_RATE) + VECTOR_GROWUP_VALUE;
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

void ruyi_vector_add_all(ruyi_vector* vector, ruyi_vector* from_vector) {
    assert(vector);
    assert(from_vector);
    INT32 i;
    INT32 len = from_vector->len;
    for (i = 0; i < len; i++) {
        ruyi_vector_add(vector, from_vector->value_data[i]);
    }
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

INT32 ruyi_vector_find_first(ruyi_vector* vector, ruyi_value value) {
    assert(vector);
    INT32 i;
    INT32 len = vector->len;
    for (i = 0; i < len; i++) {
        if (ruyi_value_equals(vector->value_data[i], value)) {
            return i;
        }
    }
    return -1;
}

INT32 ruyi_vector_find_last(ruyi_vector* vector, ruyi_value value) {
    assert(vector);
    INT32 i;
    for (i = (INT32)(vector->len) - 1; i >= 0; i--) {
        if (ruyi_value_equals(vector->value_data[i], value)) {
            return i;
        }
    }
    return -1;
}

BOOL ruyi_vector_remove_last(ruyi_vector* vector, ruyi_value* ret_last_value) {
    assert(vector);
    if (vector->len == 0) {
        return FALSE;
    }
    if (ret_last_value) {
        *ret_last_value = vector->value_data[vector->len-1];
    }
    vector->len--;
    return TRUE;
}

void ruyi_vector_sort(ruyi_vector* vector, ruyi_value_comparator comparator) {
    assert(vector);
    qsort(vector->value_data, vector->len, sizeof(ruyi_value), (int (*)(const void *, const void *))comparator);
}
