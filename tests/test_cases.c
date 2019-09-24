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
#include "../src/ruyi_lexer.h"
#include "../src/ruyi_parser.h"


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
    assert(5 == ruyi_vector_length(vector));
    values[0] = 10; values[1] = 111; values[2] = 150; values[3] = 333; values[4] = 2200;
    assert_vector_values(vector, values, 5);

    ruyi_vector_destroy(vector);
}

static void test_hashtable_it(ruyi_hashtable_iterator *it, ruyi_value *keys, ruyi_value *values, UINT32 length) {
    ruyi_hashtable * hashtable = ruyi_hashtable_create();
    UINT32 i, n;
    ruyi_value key, value, value2;
    for (i = 0 ; i < length; i++) {
        ruyi_hashtable_put(hashtable, keys[i], values[i]);
    }
    n = 0;
    while (ruyi_hashtable_iterator_next(it, &key, &value)) {
        if (!ruyi_hashtable_get(hashtable, key, &value2)) {
            assert(0);
        }
        assert(ruyi_value_equals(value, value2));
        n++;
    }
    assert(n == length);
    ruyi_hashtable_destory(hashtable);
}

static void test_hashtable(void) {
    ruyi_hashtable * hashtable = ruyi_hashtable_create();
    ruyi_value value;
    BOOL success;
    ruyi_hashtable_iterator it;
    ruyi_value keys[10];
    ruyi_value values[10];
    
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
    keys[0] = ruyi_value_str("name1"); values[0] = ruyi_value_int64(999);
    keys[1] = ruyi_value_str("name2"); values[1] = ruyi_value_int64(222);
    keys[2] = ruyi_value_str("weight"); values[2] = ruyi_value_int64(160);
    keys[3] = ruyi_value_str("number"); values[3] = ruyi_value_int64(1234);
    keys[4] = ruyi_value_str("tall"); values[4] = ruyi_value_int64(888);
    keys[5] = ruyi_value_str("age"); values[5] = ruyi_value_int64(12);

    test_hashtable_it(&it, keys, values, 6);
   
    ruyi_hashtable_clear(hashtable);
    assert(0 == ruyi_hashtable_length(hashtable));
    
    ruyi_hashtable_put(hashtable, ruyi_value_str("name1"), ruyi_value_int64(112));
    assert(1 == ruyi_hashtable_length(hashtable));
    success = ruyi_hashtable_get(hashtable, ruyi_value_str("name1"), &value);
    assert(success);
    assert(112 == value.data.int64_value);
    
    ruyi_hashtable_put(hashtable, ruyi_value_str("name6"), ruyi_value_int64(666));

    ruyi_hashtable_iterator_get(hashtable, &it);
    
    keys[0] = ruyi_value_str("name1"); values[0] = ruyi_value_int64(112);
    keys[1] = ruyi_value_str("name6"); values[1] = ruyi_value_int64(666);

    test_hashtable_it(&it, keys, values, 2);
    ruyi_hashtable_destory(hashtable);
}

static void test_hashtable_unicode_str(void) {
    ruyi_hashtable * hashtable = ruyi_hashtable_create();
    ruyi_unicode_string *us1;
    ruyi_unicode_string *us2;
    ruyi_value value;
    BOOL result;
    
    us1 = ruyi_unicode_string_init_from_utf8("name1", 0);
    us2 = ruyi_unicode_string_init_from_utf8("name22", 0);
    
    
    ruyi_hashtable_put(hashtable, ruyi_value_unicode_str(us1), ruyi_value_int64(111));
    assert(1 == ruyi_hashtable_length(hashtable));
    ruyi_hashtable_put(hashtable, ruyi_value_unicode_str(us2), ruyi_value_int64(222));
    assert(2 == ruyi_hashtable_length(hashtable));
    
    result = ruyi_hashtable_get(hashtable,  ruyi_value_unicode_str(us1), &value);
    assert(result);
    assert(111 == value.data.int64_value);
    result = ruyi_hashtable_get(hashtable,  ruyi_value_unicode_str(us2), &value);
    assert(result);
    assert(222 == value.data.int64_value);

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
    const char* file_name = "your path";
    const char* out_file = "your path";
    FILE* fp = fopen(file_name, "rb");
    FILE* fout = fopen(out_file, "w+");
    assert(fp);
    ruyi_unicode_file *file = ruyi_io_unicode_file_open(ruyi_file_open_by_file(fp));
    WIDE_CHAR buf[16];
    BYTE temp[512];
    UINT32 readCount = 0;
   // UINT32 i = 0;
    while (TRUE) {
        readCount = ruyi_io_unicode_file_read_utf8(file, buf, sizeof(buf)/sizeof(*buf));
        if (readCount == 0) {
            break;
        }
        
      //  writeCount = ruyi_io_write_utf8(fout, buf, readCount);
       // printf("writeCount: %d\n", writeCount);
        
        readCount = ruyi_unicode_encode_utf8(buf, readCount, NULL, temp, sizeof(temp)/sizeof(*temp));
        temp[readCount] = '\0';
        
        printf("%s", temp);
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

static
BOOL double_equals(double v1, double v2) {
    double v3 =  v1 - v2;
    return v3 >= -0.000001 && v3 <=  0.000001;
}

static
void assert_lexer_token(ruyi_vector * vector, UINT32 index, ruyi_token_type type, const char* input, INT64 int_value, double float_value) {
    ruyi_token *token;
    ruyi_unicode_string * temp;
    ruyi_value val;
    ruyi_vector_get(vector, index, &val);
    token = (ruyi_token *)val.data.ptr;
    assert(token->type == type);
    switch (token->type) {
        case Ruyi_tt_IDENTITY:
        case Ruyi_tt_STRING:
            if (input) {
                temp = ruyi_unicode_string_init_from_utf8(input, 0);
                assert(ruyi_unicode_string_equals(token->value.str_value, temp));
                ruyi_unicode_string_destroy(temp);
            }
            break;
        case Ruyi_tt_INTEGER:
        case Ruyi_tt_CHAR:
            assert(token->value.int_value == int_value);
            break;
        case Ruyi_tt_FLOAT:
            assert(double_equals(token->value.float_value, float_value));
            break;
        default:
            break;
    }
    
    
}

void test_lexer_id_number(void) {
    const char* src = "hello  124 world 5412.455 0x123 0b1101 0431 0 0.0 . .1230 1.23e6 4.56e-12 12e3 a .. ... .1";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_token *token;
    UINT32 i;
    ruyi_value val;
    ruyi_vector * vector = ruyi_vector_create();
    for (;;) {
        token = ruyi_lexer_reader_next_token(reader);
        if (!token) {
            printf("read next token error\n");
            break;
        }
        ruyi_vector_add(vector, ruyi_value_ptr(token));
        if (token->type == Ruyi_tt_END) {
            break;
        }
    }
    ruyi_lexer_reader_close(reader);
    assert(20 == ruyi_vector_length(vector));
    assert_lexer_token(vector, 0, Ruyi_tt_IDENTITY, "hello", 0, 0);
    assert_lexer_token(vector, 1, Ruyi_tt_INTEGER, NULL, 124, 0);
    assert_lexer_token(vector, 2, Ruyi_tt_IDENTITY, "world", 0, 0);
    assert_lexer_token(vector, 3, Ruyi_tt_FLOAT, NULL, 0, 5412.455);
    assert_lexer_token(vector, 4, Ruyi_tt_INTEGER, NULL, 0x123, 0);
    assert_lexer_token(vector, 5, Ruyi_tt_INTEGER, NULL, 0b1101, 0);
    assert_lexer_token(vector, 6, Ruyi_tt_INTEGER, NULL, 0431, 0);
    assert_lexer_token(vector, 7, Ruyi_tt_INTEGER, NULL, 0, 0);
    assert_lexer_token(vector, 8, Ruyi_tt_FLOAT, NULL, 0, 0);
    assert_lexer_token(vector, 9, Ruyi_tt_DOT, NULL, 0, 0);
    assert_lexer_token(vector, 10, Ruyi_tt_FLOAT, NULL, 0, 0.123);
    assert_lexer_token(vector, 11, Ruyi_tt_FLOAT, NULL, 0, 1.23e6);
    assert_lexer_token(vector, 12, Ruyi_tt_FLOAT, NULL, 0, 4.56e-12);
    assert_lexer_token(vector, 13, Ruyi_tt_INTEGER, NULL, 12e3, 0);
    assert_lexer_token(vector, 14, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, 15, Ruyi_tt_DOT, NULL, 0, 0);
    assert_lexer_token(vector, 16, Ruyi_tt_DOT, NULL, 0, 0);
    assert_lexer_token(vector, 17, Ruyi_tt_DOT3, NULL, 0, 0);
    assert_lexer_token(vector, 18, Ruyi_tt_FLOAT, NULL, 0, 0.1);
    assert_lexer_token(vector, 19, Ruyi_tt_END, NULL, 0, 0);
    for (i = 0; i < ruyi_vector_length(vector); i++) {
        ruyi_vector_get(vector, i, &val);
        token = (ruyi_token *)val.data.ptr;
        ruyi_lexer_token_destroy(token);
    }
    
    ruyi_vector_destroy(vector);
}

void test_lexer_id_number_string_char_comments(void) {
    const char* src = "hello 124 \"5412sksx\" \"你的x\ny\txxxa\\\"az\" 'a' '\\t' '\\'' '中' //line comments\n /* asdsd \nxxx\n yyy*/"
    " / +ab ++ += - -- -= * *= /= % %=";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_token *token;
    UINT32 i;
    ruyi_value val;
    ruyi_vector * vector = ruyi_vector_create();
    for (;;) {
        token = ruyi_lexer_reader_next_token(reader);
        if (!token) {
            printf("read next token error\n");
            break;
        }
        ruyi_vector_add(vector, ruyi_value_ptr(token));
        if (token->type == Ruyi_tt_END) {
            break;
        }
    }
    ruyi_lexer_reader_close(reader);
    i = 0;
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "hello", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_INTEGER, NULL, 124, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_STRING, "5412sksx", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_STRING, "你的x\ny\txxxa\"az", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_CHAR, NULL, 'a', 0);
    assert_lexer_token(vector, i++, Ruyi_tt_CHAR, NULL, '\t', 0);
    assert_lexer_token(vector, i++, Ruyi_tt_CHAR, NULL, '\'', 0);
    assert_lexer_token(vector, i++, Ruyi_tt_CHAR, NULL, ruyi_unicode_wide_char_utf8("中"), 0);
    assert_lexer_token(vector, i++, Ruyi_tt_LINE_COMMENTS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_MLINES_COMMENTS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_DIV, NULL, 0, 0);
    
    assert_lexer_token(vector, i++, Ruyi_tt_ADD, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "ab", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_INC, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_ADD_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_SUB, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_DEC, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_SUB_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_MUL, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_MUL_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_DIV_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_MOD, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_MOD_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_END, NULL, 0, 0);
    assert(i == ruyi_vector_length(vector));
    
    for (i = 0; i < ruyi_vector_length(vector); i++) {
        ruyi_vector_get(vector, i, &val);
        token = (ruyi_token *)val.data.ptr;
        ruyi_lexer_token_destroy(token);
    }
    
    ruyi_vector_destroy(vector);
}

void test_lexer_symbols(void) {
    const char* src = "={a|b?(a|=b) || a&b a&=b && a<b << a<<=b > >= >> >>= a==b != ! ^ ~ [x]} : :=";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_token *token;
    UINT32 i;
    ruyi_value val;
    ruyi_vector * vector = ruyi_vector_create();
    for (;;) {
        token = ruyi_lexer_reader_next_token(reader);
        if (!token) {
            printf("read next token error\n");
            break;
        }
        ruyi_vector_add(vector, ruyi_value_ptr(token));
        if (token->type == Ruyi_tt_END) {
            break;
        }
    }
    ruyi_lexer_reader_close(reader);
    i = 0;
    assert_lexer_token(vector, i++, Ruyi_tt_ASSIGN, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_LBRACE, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_BIT_OR, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "b", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_QM, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_LPAREN, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_BIT_OR_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "b", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_RPAREN, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_LOGIC_OR, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_BIT_AND, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "b", 0, 0);
    
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_BIT_AND_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "b", 0, 0);
    
    assert_lexer_token(vector, i++, Ruyi_tt_LOGIC_AND, NULL, 0, 0);

    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_LT, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "b", 0, 0);
    
    assert_lexer_token(vector, i++, Ruyi_tt_SHFT_LEFT, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_SHFT_LEFT_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "b", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_GT, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_GTE, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_SHFT_RIGHT, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_SHFT_RIGHT_ASS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "a", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_EQUALS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "b", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_NOT_EQUALS, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_LOGIC_NOT, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_BIT_XOR, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_BIT_INVERSE, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_LBRACKET, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "x", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_RBRACKET, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_RBRACE, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_COLON, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_COLON_ASSIGN, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_END, NULL, 0, 0);
    
    assert(i == ruyi_vector_length(vector));

    for (i = 0; i < ruyi_vector_length(vector); i++) {
        ruyi_vector_get(vector, i, &val);
        token = (ruyi_token *)val.data.ptr;
        ruyi_lexer_token_destroy(token);
    }
    
    ruyi_vector_destroy(vector);
}


void test_lexer_keywords(void) {
    const char* src = "hello \"abc\" if for";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_token *token;
    UINT32 i;
    ruyi_value val;
    ruyi_vector * vector = ruyi_vector_create();
    for (;;) {
        token = ruyi_lexer_reader_next_token(reader);
        if (!token) {
            printf("read next token error\n");
            break;
        }
        ruyi_vector_add(vector, ruyi_value_ptr(token));
        if (token->type == Ruyi_tt_END) {
            break;
        }
    }
    ruyi_lexer_reader_close(reader);
    i = 0;
    assert_lexer_token(vector, i++, Ruyi_tt_IDENTITY, "hello", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_STRING, "abc", 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_KW_IF, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_KW_FOR, NULL, 0, 0);
    assert_lexer_token(vector, i++, Ruyi_tt_END, NULL, 0, 0);
    
    assert(5 == ruyi_vector_length(vector));
    
    for (i = 0; i < ruyi_vector_length(vector); i++) {
        ruyi_vector_get(vector, i, &val);
        token = (ruyi_token *)val.data.ptr;
        ruyi_lexer_token_destroy(token);
    }
    
    ruyi_vector_destroy(vector);
}


void test_unicode_string(void) {
    ruyi_value v2, v3;
    ruyi_unicode_string *us1 = ruyi_unicode_string_init_from_utf8("abc中午123", 0);
    ruyi_unicode_string *us2 = ruyi_unicode_string_init_from_utf8("abc中午123", 0);
    ruyi_unicode_string *us3 = ruyi_unicode_string_init_from_utf8("abc中午123", 0);
    ruyi_bytes_string* s1 = NULL;
    assert(8 == ruyi_unicode_string_length(us1));
    ruyi_unicode_string_append_utf8(us1, "我的世界456测试啦啦4567", 0);
    assert(23 == ruyi_unicode_string_length(us1));
    s1 = ruyi_unicode_string_decode_utf8(us1);
    
    assert(0 == strncmp(s1->str, "abc中午123我的世界456测试啦啦4567", strlen(s1->str)));
    
    v2 = ruyi_value_unicode_str(us2);
    v3 = ruyi_value_unicode_str(us3);
    assert(ruyi_value_equals(v2, v3));
    
    ruyi_unicode_string_destroy(us1);
    ruyi_unicode_string_destroy(us2);
    ruyi_unicode_bytes_string_destroy(s1);
}

void test_parser_expression() {
    const char* src = "b := a + 2";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_ast *ast = NULL;
    ruyi_error *err = NULL;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
    }
    
    
    
    ruyi_ast_destroy(ast);
}

void run_test_cases(void) {
    test_lists();
    test_vectors();
    test_hashtable();
    test_hashtable_unicode_str();
    test_unicode();
    test_unicode_string();
  //  test_file();
  //  test_unicode_file();
    test_lexer_id_number();
    test_lexer_id_number_string_char_comments();
    test_lexer_symbols();
    test_lexer_keywords();
    
    test_parser_expression();
}
