//
//  test_cases.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/15.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "test_cases.h"
#include <stdio.h>
#include <string.h>
#include "../src/ruyi_list.h"
#include "../src/ruyi_vector.h"
#include "../src/ruyi_value.h"
#include "../src/ruyi_hashtable.h"
#include "../src/ruyi_unicode.h"
#include "../src/ruyi_io.h"



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

int int64_comp(ruyi_value *v1, ruyi_value *v2) {
    return (int)(v1->data.int64_value - v2->data.int64_value);
}

static void test_vectors(void) {
    INT64 values[16];
    INT32 pos;
    ruyi_value value;
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

    pos = ruyi_vector_find_first(vector, ruyi_value_int64(2200));
    assert(1 == pos);
    pos = ruyi_vector_find_last(vector, ruyi_value_int64(333));
    assert(2 == pos);
    pos = ruyi_vector_find_last(vector, ruyi_value_int64(8822));
    assert(pos < 0);
    
    ruyi_vector_remove_last(vector, &value);
    assert_vector_values(vector, values, 3);
    
    ruyi_vector_add(vector, ruyi_value_int64(10));
    ruyi_vector_add(vector, ruyi_value_int64(150));

    ruyi_vector_sort(vector, int64_comp);
    
    printf("=>>>>>>>>>>>>>>>>>>\n");
    for (pos = 0; pos < ruyi_vector_length(vector); pos++) {
        ruyi_vector_get(vector, pos, &value);
        printf("%lld\n", value.data.int64_value);
    }
    printf("=>>>>>>>>>>>>>>>>>>\n");

    ruyi_vector_destroy(vector);
}

static void test_hashtable(void) {
    ruyi_hashtable * hashtable = ruyi_hashtable_create();
    ruyi_value key;
    ruyi_value value;
    BOOL success;
    ruyi_hashtable_iterator it;
    
    ruyi_hashtable_put(hashtable, ruyi_value_str("name1"), ruyi_value_int64(111));
    assert(1 == ruyi_hashtable_length(hashtable));
    ruyi_hashtable_put(hashtable, ruyi_value_str("name2"), ruyi_value_int64(222));
    assert(2 == ruyi_hashtable_length(hashtable));
    ruyi_hashtable_put(hashtable, ruyi_value_str("age"), ruyi_value_int64(12));
    ruyi_hashtable_put(hashtable, ruyi_value_str("tall"), ruyi_value_int64(178));
    ruyi_hashtable_put(hashtable, ruyi_value_str("weight"), ruyi_value_int64(160));
    ruyi_hashtable_put(hashtable, ruyi_value_str("number"), ruyi_value_int64(1234));
    assert(6 == ruyi_hashtable_length(hashtable));
    
    success = ruyi_hashtable_get(hashtable, ruyi_value_str("name1"), &value);
    assert(success);
    assert(111 == value.data.int64_value);
    
    success = ruyi_hashtable_get(hashtable, ruyi_value_str("tall"), &value);
    assert(success);
    assert(178 == value.data.int64_value);
    
    ruyi_hashtable_put(hashtable, ruyi_value_str("tall"), ruyi_value_int64(888));
    assert(6 == ruyi_hashtable_length(hashtable));

    success = ruyi_hashtable_get(hashtable, ruyi_value_str("tall"), &value);
    assert(success);
    assert(888 == value.data.int64_value);

    success = ruyi_hashtable_delete(hashtable, ruyi_value_str("name1"));
    assert(success);
    assert(5 == ruyi_hashtable_length(hashtable));
    
    success = ruyi_hashtable_get(hashtable, ruyi_value_str("name1"), &value);
    assert(success == FALSE);
    
    ruyi_hashtable_put(hashtable, ruyi_value_str("name1"), ruyi_value_int64(999));
    assert(6 == ruyi_hashtable_length(hashtable));
    success = ruyi_hashtable_get(hashtable, ruyi_value_str("name1"), &value);
    assert(success);
    assert(999 == value.data.int64_value);
    
    ruyi_hashtable_iterator_get(hashtable, &it);
    while (ruyi_hashtable_iterator_next(&it, &key, &value)) {
        printf("%s ==> %lld\n", (char*)key.data.ptr, value.data.int64_value);
    }
    
    ruyi_hashtable_clear(hashtable);
    assert(0 == ruyi_hashtable_length(hashtable));
    
    ruyi_hashtable_put(hashtable, ruyi_value_str("name1"), ruyi_value_int64(112));
    assert(1 == ruyi_hashtable_length(hashtable));
    success = ruyi_hashtable_get(hashtable, ruyi_value_str("name1"), &value);
    assert(success);
    assert(112 == value.data.int64_value);
    
    ruyi_hashtable_put(hashtable, ruyi_value_str("name6"), ruyi_value_int64(666));

    printf("==========--------->>>>>>>>>>>>>>\n");
    ruyi_hashtable_iterator_get(hashtable, &it);
    while (ruyi_hashtable_iterator_next(&it, &key, &value)) {
        printf("%s ==> %lld\n", (char*)key.data.ptr, value.data.int64_value);
    }
    ruyi_hashtable_destory(hashtable);
}

void test_unicode() {
    const BYTE str[] = {0xE6,0xB1,0x89,0xE5,0xAD,0x97, 'a', 'b'};
    WIDE_CHAR ch[128];
    UINT32 result_length;
    BYTE bytes_buf[128];
    result_length = ruyi_unicode_decode_utf8(str, sizeof(str)/sizeof(*str), NULL, ch, sizeof(ch)/sizeof(*ch));
    assert(result_length == 4);
    assert(ch[0] == 0x6C49);
    assert(ch[1] == 0x5B57);
    assert(ch[2] == 'a');
    assert(ch[3] == 'b');
    
    result_length = ruyi_unicode_encode_utf8(ch, 4, NULL, bytes_buf, sizeof(bytes_buf)/sizeof(*bytes_buf));
    assert(result_length == 3 * 2 + 2);
    assert(bytes_buf[0] == (WIDE_CHAR)0xE6);
    assert(bytes_buf[1] == (WIDE_CHAR)0xB1);
    assert(bytes_buf[2] == (WIDE_CHAR)0x89);
    assert(bytes_buf[3] == (WIDE_CHAR)0xE5);
    assert(bytes_buf[4] == (WIDE_CHAR)0xAD);
    assert(bytes_buf[5] == (WIDE_CHAR)0x97);
    assert(bytes_buf[6] == 'a');
    assert(bytes_buf[7] == 'b');
}

void test_unicode_file() {
    const char* file_name = "the test input utf-8 file";
    const char* out_file = "the test output utf-8 file";
    FILE* fp = fopen(file_name, "rb");
    FILE* fout = fopen(out_file, "w+");
    assert(fp);
    ruyi_unicode_file *file = ruyi_io_unicode_file_open(fp);
    WIDE_CHAR buf[16];
    BYTE temp[512];
    UINT32 readCount = 0;
    UINT32 writeCount = 0;
   // UINT32 i = 0;
    while (TRUE) {
        readCount = ruyi_io_unicode_file_read_utf8(file, buf, sizeof(buf)/sizeof(*buf));
        if (readCount == 0) {
            break;
        }
        
        writeCount = ruyi_io_write_utf8(fout, buf, readCount);
        printf("writeCount: %d\n", writeCount);
        
        readCount = ruyi_unicode_encode_utf8(buf, readCount, NULL, temp, sizeof(temp)/sizeof(*temp));
        temp[readCount] = '\0';
        
       // printf("%s", temp);
    }
    ruyi_io_unicode_file_close(file);
    fclose(fout);
}

void test_file() {
    const char * data1 = "abcd5";
    char buf[16];
    UINT32 read_count;
    ruyi_file* file = ruyi_file_init_by_data(data1, (UINT32)strlen(data1));
    ruyi_file* file2 = ruyi_file_init_by_capacity(8);
    read_count = ruyi_file_read(file, buf, 2);
    assert(read_count == 2);
    assert(buf[0] == 'a');
    assert(buf[1] == 'b');
    read_count = ruyi_file_read(file, buf, 2);
    assert(read_count == 2);
    assert(buf[0] == 'c');
    assert(buf[1] == 'd');
    read_count = ruyi_file_read(file, buf, 2);
    assert(read_count == 1);
    assert(buf[0] == '5');
    
    data1 ="abcdef12345";
    ruyi_file_write(file2, data1, (UINT32)strlen(data1));
    data1 ="8899";
    ruyi_file_write(file2, data1, (UINT32)strlen(data1));
    read_count = ruyi_file_read(file2, buf, 6);
    assert(read_count == 6);
    assert(buf[0] == 'a');
    assert(buf[1] == 'b');
    assert(buf[2] == 'c');
    assert(buf[3] == 'd');
    assert(buf[4] == 'e');
    assert(buf[5] == 'f');
    read_count = ruyi_file_read(file2, buf, 6);
    assert(read_count == 6);
    assert(buf[0] == '1');
    assert(buf[1] == '2');
    assert(buf[2] == '3');
    assert(buf[3] == '4');
    assert(buf[4] == '5');
    assert(buf[5] == '8');
    read_count = ruyi_file_read(file2, buf, 6);
    assert(read_count == 3);
    assert(buf[0] == '8');
    assert(buf[1] == '9');
    assert(buf[2] == '9');
    
    ruyi_file_close(file);
    ruyi_file_close(file2);
}

void run_test_cases(void) {
    test_lists();
    test_vectors();
    test_hashtable();
    test_unicode();
    test_file();
   // test_unicode_file();
}
