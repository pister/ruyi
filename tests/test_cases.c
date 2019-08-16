//
//  test_cases.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "test_cases.h"
#include <stdio.h>
#include "../src/ruyi_list.h"
#include "../src/ruyi_vector.h"


BOOL print_callback(ruyi_value v) {
    printf("value: %lld\n", v.data.int64_value);
    return TRUE;
}

static void assert_list_values(ruyi_list * list, const INT64 *values, int value_length) {
    int pos = 0;
    ruyi_list_item *item = list->first;
    while (item) {
        assert(item->value.data.int64_value == values[pos]);
        item = item->next;
        pos++;
    }
    assert(pos == value_length);
}

static void test_lists(void) {
    INT64 values[16];
    ruyi_value value;
    ruyi_list * list = ruyi_list_create();
    ruyi_value val;
    ruyi_list_item* item;
    assert(ruyi_list_empty(list));
    val = ruyi_value_int64(12);
    ruyi_list_add_last(list, val);
    val = ruyi_value_int64(34);
    ruyi_list_add_last(list, val);
    val = ruyi_value_int64(88);
    ruyi_list_add_last(list, val);
    val = ruyi_value_int64(11);
    ruyi_list_add_first(list, val);
    val = ruyi_value_int64(22);
    ruyi_list_add_first(list, val);
    assert(ruyi_list_empty(list) == FALSE);
    values[0] = 22; values[1] = 11; values[2] = 12;
    values[3] = 34; values[4] = 88;
    assert_list_values(list, values, 5);
    
    ruyi_list_remove_last(list, &value);
    assert_list_values(list, values, 4);
    assert(88 == value.data.int64_value);
    
    ruyi_list_add_last(list, ruyi_value_int64(99));
    values[4]= 99;
    assert_list_values(list, values, 5);
    
    ruyi_list_remove_first(list, &value);
    values[0] = 11; values[1] = 12;
    values[2] = 34; values[3] = 99;
    assert_list_values(list, values, 4);
    
    ruyi_list_get_last(list, &value);
    assert(99 == value.data.int64_value);
    
    ruyi_list_get_first(list, &value);
    assert(11 == value.data.int64_value);
    
    item = ruyi_list_find_first(list, ruyi_value_int64(12));   // the second
    assert(item);
    ruyi_list_insert_after(list, item, ruyi_value_int64(33));
    
    values[0] = 11; values[1] = 12; values[2] = 33;
    values[3] = 34; values[4] = 99;
    assert_list_values(list, values, 5);
    
    ruyi_list_insert_before(list, item, ruyi_value_int64(331));
    
    values[0] = 11; values[1] = 331; values[2] = 12;
    values[3] = 33; values[4] = 34; values[5] = 99;
    
    assert_list_values(list, values, 6);
    
    ruyi_list_destroy(list);
}

static void assert_vector_values(ruyi_vector* vector, const INT64 *values, UINT32 value_length) {
    UINT32 i;
    ruyi_value value;
    for (i = 0; i < ruyi_vector_length(vector); i++) {
        if (!ruyi_vector_get(vector, i, &value)) {
            assert(0);
        }
        assert(value.data.int64_value == values[i]);
    }
    assert(i == value_length);
}

static void test_vectors(void) {
    INT64 values[16];
    ruyi_vector* vector = ruyi_vector_create_with_cap(2);
    ruyi_vector_add(vector, ruyi_value_int64(111));
    ruyi_vector_add(vector, ruyi_value_int64(222));
    ruyi_vector_add(vector, ruyi_value_int64(333));
    ruyi_vector_add(vector, ruyi_value_int64(444));
    values[0] = 111; values[1] = 222; values[2] = 333; values[3] = 444;
    assert_vector_values(vector, values, 4);
    
    ruyi_vector_set(vector, 1, ruyi_value_int64(2200));
    values[1] = 2200;
    assert_vector_values(vector, values, 4);

    
    ruyi_vector_destroy(vector);
}

void run_test_cases(void) {
    test_lists();
    test_vectors();
}
