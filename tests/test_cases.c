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
#include "../src/ruyi_ir.h"
#include "../src/ruyi_code_generator.h"



static BOOL print_callback(ruyi_value v) {
    printf("value: %lld\n", v.data.int64_value);
    return TRUE;
}

static void print_unicode(ruyi_unicode_string* ustr) {
    ruyi_bytes_string* bstr = ruyi_unicode_string_encode_utf8(ustr);
    if (bstr == NULL) {
        return;
    }
    printf("%s\n", bstr->str);
    ruyi_unicode_bytes_string_destroy(bstr);
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

int int64_comp(const ruyi_value *v1, const ruyi_value *v2) {
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
    ruyi_hashtable_destroy(hashtable);
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
    ruyi_hashtable_destroy(hashtable);
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

    ruyi_hashtable_destroy(hashtable);
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
    ruyi_unicode_string *us4 = ruyi_unicode_string_copy_from(us1);
    ruyi_bytes_string* s1 = NULL;
    assert(8 == ruyi_unicode_string_length(us1));
    ruyi_unicode_string_append_utf8(us1, "我的世界456测试啦啦4567", 0);
    assert(23 == ruyi_unicode_string_length(us1));
    s1 = ruyi_unicode_string_encode_utf8(us1);
    assert(0 == strncmp(s1->str, "abc中午123我的世界456测试啦啦4567", strlen(s1->str)));
    v2 = ruyi_value_unicode_str(us2);
    v3 = ruyi_value_unicode_str(us3);
    assert(ruyi_value_equals(v2, v3));
    assert(ruyi_unicode_string_equals(us2, us4));
    ruyi_unicode_string_destroy(us1);
    ruyi_unicode_string_destroy(us2);
    ruyi_unicode_string_destroy(us4);
    ruyi_unicode_bytes_string_destroy(s1);
}


void test_parser_expression() {
    const char* src = "bb := aa + 2";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_delcarations = NULL;
    ruyi_ast *assign_ast = NULL;
    ruyi_error *err = NULL;
    ruyi_unicode_string* v1 = NULL;
    ruyi_unicode_string* v2 = NULL;
    ruyi_ast *type_ast;
    ruyi_ast *expr_ast;
    ruyi_ast *var_aa_ast;
    ruyi_ast *op_ast;
    ruyi_ast *val_2_ast;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    
    assert(Ruyi_at_root == ast->type);
    assert(3 == ruyi_ast_child_length(ast));
    ast_delcarations = ruyi_ast_get_child(ast, 2);
    assign_ast = ruyi_ast_get_child(ast_delcarations, 0);
    assert(Ruyi_at_var_declaration == assign_ast->type);
    //assign_ast->data.ptr_value
    v1 = ruyi_unicode_string_init_from_utf8("bb", 0);
    assert(ruyi_unicode_string_equals(v1, (ruyi_unicode_string*)assign_ast->data.ptr_value));
    assert(2 == assign_ast->child_asts->len);
    type_ast = ruyi_ast_get_child(assign_ast, 0);
    expr_ast = ruyi_ast_get_child(assign_ast, 1);
    assert(type_ast->type = Ruyi_at_var_declaration_auto_type);
    assert(expr_ast->type = Ruyi_at_additive_expression);
    assert(3 == expr_ast->child_asts->len);

    var_aa_ast = ruyi_ast_get_child(expr_ast, 0);
    op_ast = ruyi_ast_get_child(expr_ast, 1);
    val_2_ast = ruyi_ast_get_child(expr_ast, 2);

    assert(var_aa_ast->type = Ruyi_at_name);
    v2 = ruyi_unicode_string_init_from_utf8("aa", 0);
    assert(ruyi_unicode_string_equals(v2, (ruyi_unicode_string*)var_aa_ast->data.ptr_value));
    assert(op_ast->type = Ruyi_at_op_add);
    assert(val_2_ast->type = Ruyi_at_integer);
    assert(2 == val_2_ast->data.int32_value);

    ruyi_ast_destroy(ast);
    ruyi_unicode_string_destroy(v1);
    ruyi_unicode_string_destroy(v2);
    // type_ast, expr_ast etc ... will be auto destroy by code: ruyi_ast_destroy(ast);
}

void test_parser_package_import_vars() {
    const char* src = "package bb; import a1\n import a2; \n c1 := 10; var c2 long = 20;";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_declarations = NULL;
    ruyi_ast *package_declaration = NULL;
    ruyi_ast *import_declarations = NULL;
    ruyi_ast *var_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *ast_delcarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    assert(3 == ruyi_ast_child_length(ast));
    
    import_declarations = ruyi_ast_get_child(ast, 1);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    var_ast = ruyi_ast_get_child(ast_declarations, 0);
    
    // package bb
    package_declaration = ruyi_ast_get_child(ast, 0);
    assert(package_declaration);
    assert(Ruyi_at_package_declaration == package_declaration->type);
    temp_ast = ruyi_ast_get_child(package_declaration, 0);
    name = ruyi_unicode_string_init_from_utf8("bb", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // import a1\n import a2; \n
    import_declarations = ruyi_ast_get_child(ast, 1);
    assert(import_declarations);
    assert(Ruyi_at_import_declarations == import_declarations->type);
    assert(2 == ruyi_ast_child_length(import_declarations));
    // import a1
    temp_ast = ruyi_ast_get_child(import_declarations, 0);
    assert(Ruyi_at_import_declaration == temp_ast->type);
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    name = ruyi_unicode_string_init_from_utf8("a1", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // import a2
    temp_ast = ruyi_ast_get_child(import_declarations, 1);
    assert(Ruyi_at_import_declaration == temp_ast->type);
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    name = ruyi_unicode_string_init_from_utf8("a2", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // global
    ast_delcarations = ruyi_ast_get_child(ast, 2);
    
    assert(2 == ruyi_ast_child_length(ast_delcarations));
    
    // c1 := 10
    temp_ast = ruyi_ast_get_child(ast_delcarations, 0);
    assert(Ruyi_at_var_declaration == temp_ast->type);
    name = ruyi_unicode_string_init_from_utf8("c1", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    // auto-type
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_var_declaration_auto_type == temp_ast2->type);
    // 10
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_integer == temp_ast2->type);
    assert(10 == temp_ast2->data.int32_value);
    
    // var c2 long = 20
    temp_ast = ruyi_ast_get_child(ast_delcarations, 1);
    assert(Ruyi_at_var_declaration == temp_ast->type);
    name = ruyi_unicode_string_init_from_utf8("c2", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    // long
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_type_long == temp_ast2->type);
    // 20
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_integer == temp_ast2->type);
    assert(20 == temp_ast2->data.int32_value);
    
    
    ruyi_ast_destroy(ast);
}

void test_parser_function_return() {
    const char* src = "func add1(a int, b long) long { return a*10 + b;}";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *ast_declarations = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;

    UINT32 len;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    assert(3 == ruyi_ast_child_length(ast));
    ast_declarations = ruyi_ast_get_child(ast, 2);
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // assert name
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    name = ruyi_unicode_string_init_from_utf8("add1", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);

    // assert formal_params
    temp_ast = ruyi_ast_get_child(func_ast, 1);
    assert(temp_ast != NULL);
    assert(Ruyi_at_formal_parameter_list == temp_ast->type);
    len = ruyi_ast_child_length(temp_ast);
    assert(2 == len);
    
    // a int
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_formal_parameter == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast2, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(ruyi_ast_get_child(temp_ast2, 1)->type = Ruyi_at_type_int);
    
    // b long
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_formal_parameter == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast2, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(ruyi_ast_get_child(temp_ast2, 1)->type = Ruyi_at_type_long);
    
    // return type
    temp_ast2 = ruyi_ast_get_child(func_ast, 2);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_type_list == temp_ast2->type);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    assert(Ruyi_at_type_long == ruyi_ast_get_child(temp_ast2, 0)->type);
    
    // body
    temp_ast2 = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_block_statements == temp_ast2->type);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_return_statement == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    
    // return expression: a*10 + b
    temp_ast2 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_additive_expression == temp_ast2->type);
    assert(3 == ruyi_ast_child_length(temp_ast2));
    
    // a*10
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_multiplicative_expression == temp_ast3->type);
    assert(3 == ruyi_ast_child_length(temp_ast3));
    
    temp_ast = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_name == temp_ast->type);
    
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_mul == ruyi_ast_get_child(temp_ast3, 1)->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast3, 2)->type);
    assert(10 == ruyi_ast_get_child(temp_ast3, 2)->data.int64_value);

    // +
    assert(Ruyi_at_op_add == ruyi_ast_get_child(temp_ast2, 1)->type);
    
    // b
    temp_ast2 = ruyi_ast_get_child(temp_ast2, 2);
    assert(Ruyi_at_name == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);

    ruyi_ast_destroy(ast);
}


void test_parser_function_if() {
    const char* src = "func if1(a int, b long) long { if (a > 100.0) {return 100;} elseif (a > 50) {return 50;} elseif (a > b) {return a;}  else { return b;} }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;
    ruyi_ast *ast_declarations;
    
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // assert name
    name = ruyi_unicode_string_init_from_utf8("if1", 0);
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // assert formal_params
    temp_ast = ruyi_ast_get_child(func_ast, 1);
    assert(temp_ast != NULL);
    assert(Ruyi_at_formal_parameter_list == temp_ast->type);
    assert(2 == ruyi_ast_child_length(temp_ast));
    
    // a int
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_formal_parameter == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast2, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(ruyi_ast_get_child(temp_ast2, 1)->type = Ruyi_at_type_int);
    
    // b long
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_formal_parameter == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast2, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_type_long == ruyi_ast_get_child(temp_ast2, 1)->type);
    
    // return type
    temp_ast2 = ruyi_ast_get_child(func_ast, 2);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_type_list == temp_ast2->type);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    assert(Ruyi_at_type_long == ruyi_ast_get_child(temp_ast2, 0)->type);

    
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(1 == ruyi_ast_child_length(temp_ast));
    
    // if stmt
    temp_ast = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_if_statement == temp_ast->type);
    /*
    ast struct:
     chidren:
     0 - if expr
     1 - if block
     2 - n ? : elseif_ast
     n + 1: else_ast
     */
    assert(5 == ruyi_ast_child_length(temp_ast));
    // if (a > 100.0) { return 100; }
    // expr: a > 100
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_relational_expression == temp_ast2->type);
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast2, 0)->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast2, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_gt == ruyi_ast_get_child(temp_ast2, 1)->type);
    assert(Ruyi_at_float == ruyi_ast_get_child(temp_ast2, 2)->type);
    // block {return 100;}
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_block_statements == temp_ast2->type);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    // return 100;
    temp_ast2 = ruyi_ast_get_child(temp_ast2, 0);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_integer == temp_ast3->type);
    assert(100 == ruyi_ast_get_child(temp_ast2, 0)->data.int32_value);

    // elseif (a > 50) { return 50; }
    temp_ast2 = ruyi_ast_get_child(temp_ast, 2);
    assert(Ruyi_at_elseif_statement == temp_ast2->type);
    assert(2 == ruyi_ast_child_length(temp_ast2));
    // expr a > 50
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_relational_expression == temp_ast3->type);
    assert(3 == ruyi_ast_child_length(temp_ast3));
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast3, 0)->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast3, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_gt == ruyi_ast_get_child(temp_ast3, 1)->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast3, 2)->type);
    assert(50 == ruyi_ast_get_child(temp_ast3, 2)->data.int32_value);
    // return 50;
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 1);
    assert(Ruyi_at_block_statements == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    temp_ast3 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_return_statement == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast3, 0)->type);
    assert(50 == ruyi_ast_get_child(temp_ast3, 0)->data.int32_value);

    // elseif (a > b) { return a; }
    temp_ast2 = ruyi_ast_get_child(temp_ast, 3);
    assert(Ruyi_at_elseif_statement == temp_ast2->type);
    assert(2 == ruyi_ast_child_length(temp_ast2));
    // expr a > b
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_relational_expression == temp_ast3->type);
    assert(3 == ruyi_ast_child_length(temp_ast3));
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast3, 0)->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast3, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_gt == ruyi_ast_get_child(temp_ast3, 1)->type);
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast3, 2)->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast3, 2)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // return a;
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 1);
    assert(Ruyi_at_block_statements == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    temp_ast3 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_return_statement == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast3, 0)->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast3, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // else { return b;}
    temp_ast2 = ruyi_ast_get_child(temp_ast, 4);
    assert(Ruyi_at_else_statement == temp_ast2->type);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    // return b;
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_block_statements == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    temp_ast3 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_return_statement == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast3, 0)->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast3, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    ruyi_ast_destroy(ast);
}


void test_parser_function_while() {
    // TODO use a++ instead of return a+1
    const char* src = "func while1(a int) int { while(a < 10) {a++;} return a; }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;
    ruyi_ast *temp_ast4 = NULL;
    ruyi_ast *ast_declarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // assert name
    name = ruyi_unicode_string_init_from_utf8("while1", 0);
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // assert formal_params
    temp_ast = ruyi_ast_get_child(func_ast, 1);
    assert(temp_ast != NULL);
    assert(Ruyi_at_formal_parameter_list == temp_ast->type);
    assert(1 == ruyi_ast_child_length(temp_ast));
    
    // a int
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_formal_parameter == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast2, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(ruyi_ast_get_child(temp_ast2, 1)->type = Ruyi_at_type_int);
    
    // return type
    temp_ast2 = ruyi_ast_get_child(func_ast, 2);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_type_list == temp_ast2->type);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    assert(Ruyi_at_type_int == ruyi_ast_get_child(temp_ast2, 0)->type);
    
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(2 == ruyi_ast_child_length(temp_ast));
    
    // while stmt:
    // while(a < 10) {a++;}
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_while_statement == temp_ast2->type);
    assert(2 == ruyi_ast_child_length(temp_ast2));
    // expr: a < 10
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_relational_expression == temp_ast3->type);
    // body: {a++}
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 1);
    assert(Ruyi_at_block_statements == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    // {a++;}
    temp_ast3 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_left_hand_side_expression == temp_ast3->type);
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_name == temp_ast4->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast4->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 1);
    assert(Ruyi_at_inc_statement == temp_ast4->type);
    // return a;
    temp_ast3 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_return_statement == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_name == temp_ast4->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast4->data.ptr_value));
    ruyi_unicode_string_destroy(name);
   
    ruyi_ast_destroy(ast);
}

void test_parser_function_for() {
    const char* src = "func for1() {\n sum := 0 \n for (i := 0; i < 10; i++) {\n sum = sum + i;} \n for(a1, a2 in b) { sum = a1 + a2; } }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;
    ruyi_ast *temp_ast4 = NULL;
    ruyi_ast *temp_ast5 = NULL;
    ruyi_ast *ast_declarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // assert name
    name = ruyi_unicode_string_init_from_utf8("for1", 0);
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // assert formal_params
    temp_ast = ruyi_ast_get_child(func_ast, 1);
    assert(temp_ast != NULL);
    assert(Ruyi_at_formal_parameter_list == temp_ast->type);
    assert(0 == ruyi_ast_child_length(temp_ast));
    // return type
    temp_ast2 = ruyi_ast_get_child(func_ast, 2);
    assert(temp_ast2 == NULL);
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(3 == ruyi_ast_child_length(temp_ast));
    
    // sum := 0
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_var_declaration == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("sum", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_var_declaration_auto_type == ruyi_ast_get_child(temp_ast2, 0)->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast2, 1)->type);
    assert(0 == ruyi_ast_get_child(temp_ast2, 1)->data.int32_value);

    // for (i := 0; i < 10; i++) {\n sum = sum + i;}
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_for_3_parts_statement == temp_ast2->type);
    // for (i := 0; i < 10; i++)
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_for_3_parts_header == temp_ast3->type);
    // i := 0;
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_expr_statement_list == temp_ast4->type);
    assert(1 == ruyi_ast_child_length(temp_ast4));
    temp_ast4 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_var_declaration == temp_ast4->type);
    name = ruyi_unicode_string_init_from_utf8("i", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast4->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_var_declaration_auto_type == ruyi_ast_get_child(temp_ast4, 0)->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast4, 1)->type);
    assert(0 == ruyi_ast_get_child(temp_ast4, 1)->data.int32_value);
    // i < 10;
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 1);
    assert(Ruyi_at_relational_expression == temp_ast4->type);
    name = ruyi_unicode_string_init_from_utf8("i", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast4, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_lt == ruyi_ast_get_child(temp_ast4, 1)->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast4, 2)->type);
    assert(10 == ruyi_ast_get_child(temp_ast4, 2)->data.int32_value);
    // i++;
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 2);
    assert(Ruyi_at_stmt_expr_list == temp_ast4->type);
    
    // { sum = sum + i;}
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 1);
    assert(Ruyi_at_block_statements == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_left_hand_side_expression == temp_ast4->type);
    // sum
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_name == temp_ast5->type);
    name = ruyi_unicode_string_init_from_utf8("sum", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast5->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // = sum + i
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 1);
    assert(Ruyi_at_assign_statement == temp_ast5->type);
    temp_ast5 = ruyi_ast_get_child(temp_ast5, 0);
    assert(Ruyi_at_additive_expression == temp_ast5->type);
    name = ruyi_unicode_string_init_from_utf8("sum", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast5, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_add == ruyi_ast_get_child(temp_ast5, 1)->type);
    name = ruyi_unicode_string_init_from_utf8("i", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast5, 2)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // for(a1, a2 in b) { sum = a1 + a2; }
    temp_ast2 = ruyi_ast_get_child(temp_ast, 2);
    assert(Ruyi_at_for_in_statement == temp_ast2->type);
    // for(a1, a2 in b)
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_for_in_header == temp_ast3->type);
    // a1, a2
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_var_list == temp_ast4->type);
    assert(2 == ruyi_ast_child_length(temp_ast4));
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast4, 0)->type);
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast4, 1)->type);
    name = ruyi_unicode_string_init_from_utf8("a1", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast4, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    name = ruyi_unicode_string_init_from_utf8("a2", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast4, 1)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // b
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 1);
    assert(Ruyi_at_name == temp_ast4->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast4->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // { sum = a1 + a2; }
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 1);
    assert(Ruyi_at_block_statements == temp_ast3->type);
    assert(1 == ruyi_ast_child_length(temp_ast3));
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_left_hand_side_expression == temp_ast4->type);
    // sum
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_name == temp_ast5->type);
    name = ruyi_unicode_string_init_from_utf8("sum", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast5->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // = a1 + a2;
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 1);
    assert(Ruyi_at_assign_statement == temp_ast5->type);
    temp_ast5 = ruyi_ast_get_child(temp_ast5, 0);
    assert(Ruyi_at_additive_expression == temp_ast5->type);
    name = ruyi_unicode_string_init_from_utf8("a1", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast5, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_add == ruyi_ast_get_child(temp_ast5, 1)->type);
    name = ruyi_unicode_string_init_from_utf8("a2", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast5, 2)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    ruyi_ast_destroy(ast);
}

void test_parser_function_switch(void) {
    const char* src = "func switch1(v int) {var b int; switch(v) {case 1,2,3: b=10;\n case 12+5: b = 20;\n default: b = 100} }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;
    ruyi_ast *temp_ast4 = NULL;
    ruyi_ast *temp_ast5 = NULL;
    ruyi_ast *temp_ast6 = NULL;
    ruyi_ast *temp_ast7 = NULL;
    ruyi_ast *temp_ast8 = NULL;
    ruyi_ast *ast_declarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // assert name
    name = ruyi_unicode_string_init_from_utf8("switch1", 0);
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // assert formal_params
    temp_ast = ruyi_ast_get_child(func_ast, 1);
    assert(temp_ast != NULL);
    assert(Ruyi_at_formal_parameter_list == temp_ast->type);
    assert(1 == ruyi_ast_child_length(temp_ast));
    
    // v int
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(temp_ast2 != NULL);
    assert(Ruyi_at_formal_parameter == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("v", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast2, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(ruyi_ast_get_child(temp_ast2, 1)->type = Ruyi_at_type_int);
    
    // return type
    temp_ast2 = ruyi_ast_get_child(func_ast, 2);
    assert(temp_ast2 == NULL);
    
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(2 == ruyi_ast_child_length(temp_ast));
    
    // var b int;
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_var_declaration == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_type_int == temp_ast3->type);
    
    // switch(v) {case 1,2,3: b=10;\n case 12+5: b = 20;\n default: b = 100}
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_switch_statement == temp_ast2->type);
    // v
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_name == temp_ast3->type);
    // {case 1,2,3: b=10;\n case 12+5: b = 20;\n default: b = 100}
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 1);
    assert(Ruyi_at_switch_statement_body == temp_ast3->type);
    assert(2 == ruyi_ast_child_length(temp_ast3));
    // case 1,2,3: b=10;\n case 12+5: b = 20;
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_switch_case_statement_list == temp_ast4->type);
    assert(2 == ruyi_ast_child_length(temp_ast4));
    // case 1,2,3: b=10;
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_switch_case_statement == temp_ast5->type);
    assert(2 == ruyi_ast_child_length(temp_ast5));
    // 1,2,3:
    temp_ast6 = ruyi_ast_get_child(temp_ast5, 0);
    assert(Ruyi_at_const_list == temp_ast6->type);
    assert(3 == ruyi_ast_child_length(temp_ast6));
    // 1
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 0);
    assert(Ruyi_at_constant_expression == temp_ast7->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast7, 0)->type);
    assert(1 == ruyi_ast_get_child(temp_ast7, 0)->data.int32_value);
    // 2
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 1);
    assert(Ruyi_at_constant_expression == temp_ast7->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast7, 0)->type);
    assert(2 == ruyi_ast_get_child(temp_ast7, 0)->data.int32_value);
    // 3
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 2);
    assert(Ruyi_at_constant_expression == temp_ast7->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast7, 0)->type);
    assert(3 == ruyi_ast_get_child(temp_ast7, 0)->data.int32_value);
    // b=10;
    temp_ast6 = ruyi_ast_get_child(temp_ast5, 1);
    assert(Ruyi_at_block_statements == temp_ast6->type);
    assert(1 == ruyi_ast_child_length(temp_ast6));
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 0);
    assert(Ruyi_at_left_hand_side_expression == temp_ast7->type);
    // b
    temp_ast8 = ruyi_ast_get_child(temp_ast7, 0);
    assert(Ruyi_at_name == temp_ast8->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast8->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // =10
    temp_ast8 = ruyi_ast_get_child(temp_ast7, 1);
    assert(Ruyi_at_assign_statement == temp_ast8->type);
    assert(1 == ruyi_ast_child_length(temp_ast8));
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast8, 0)->type);
    assert(10 == ruyi_ast_get_child(temp_ast8, 0)->data.int32_value);
    // case 12+5: b = 20;
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 1);
    assert(Ruyi_at_switch_case_statement == temp_ast5->type);
    assert(2 == ruyi_ast_child_length(temp_ast5));
    // 12+5:
    temp_ast6 = ruyi_ast_get_child(temp_ast5, 0);
    assert(Ruyi_at_const_list == temp_ast6->type);
    assert(1 == ruyi_ast_child_length(temp_ast6));
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 0);
    assert(Ruyi_at_constant_expression == temp_ast7->type);
    temp_ast8 = ruyi_ast_get_child(temp_ast7, 0);
    assert(Ruyi_at_additive_expression == temp_ast8->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast8, 0)->type);
    assert(Ruyi_at_op_add == ruyi_ast_get_child(temp_ast8, 1)->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast8, 2)->type);
    assert(12 == ruyi_ast_get_child(temp_ast8, 0)->data.int32_value);
    assert(5 == ruyi_ast_get_child(temp_ast8, 2)->data.int32_value);
    // b = 20;
    temp_ast6 = ruyi_ast_get_child(temp_ast5, 1);
    assert(Ruyi_at_block_statements == temp_ast6->type);
    assert(1 == ruyi_ast_child_length(temp_ast6));
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 0);
    assert(Ruyi_at_left_hand_side_expression == temp_ast7->type);
    // b
    temp_ast8 = ruyi_ast_get_child(temp_ast7, 0);
    assert(Ruyi_at_name == temp_ast8->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast8->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // = 20
    temp_ast8 = ruyi_ast_get_child(temp_ast7, 1);
    assert(Ruyi_at_assign_statement == temp_ast8->type);
    assert(1 == ruyi_ast_child_length(temp_ast8));
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast8, 0)->type);
    assert(20 == ruyi_ast_get_child(temp_ast8, 0)->data.int32_value);
    
    // default: b = 100
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 1);
    assert(Ruyi_at_switch_default_case_statement == temp_ast4->type);
    // b = 100
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_block_statements == temp_ast5->type);

    ruyi_ast_destroy(ast);
}

void test_parser_function_break_continue() {
    const char* src = "func break_continue1() {break ab; continue; }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *ast_declarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // assert name
    name = ruyi_unicode_string_init_from_utf8("break_continue1", 0);
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(2 == ruyi_ast_child_length(temp_ast));
    
    // break ab;
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_break_statement == temp_ast2->type);
    assert(NULL != temp_ast2->data.ptr_value);
    name = ruyi_unicode_string_init_from_utf8("ab", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // continue;
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_continue_statement == temp_ast2->type);
    assert(NULL == temp_ast2->data.ptr_value);
    
    ruyi_ast_destroy(ast);
}

void test_parser_function_label(void) {
    const char* src = "func label_1() { ab:while(1) {break ab;} }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;
    ruyi_ast *temp_ast4 = NULL;
    ruyi_ast *temp_ast5 = NULL;
    ruyi_ast *ast_declarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // assert name
    name = ruyi_unicode_string_init_from_utf8("label_1", 0);
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(1 == ruyi_ast_child_length(temp_ast));
    
    // ab:while(1) {break ab;}
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_labeled_statement == temp_ast2->type);
    assert(NULL != temp_ast2->data.ptr_value);
    name = ruyi_unicode_string_init_from_utf8("ab", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // while(1) {break ab;}
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_while_statement == temp_ast3->type);
    assert(2 == ruyi_ast_child_length(temp_ast3));
    // expr: 1
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_integer == temp_ast4->type);
    assert(1 == temp_ast4->data.int32_value);
    // body: {break ab;}
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 1);
    assert(Ruyi_at_block_statements == temp_ast4->type);
    assert(1 == ruyi_ast_child_length(temp_ast4));
    // {break ab;}
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_break_statement == temp_ast5->type);
    name = ruyi_unicode_string_init_from_utf8("ab", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast5->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    ruyi_ast_destroy(ast);
}

void test_parser_function_sub_block(void) {
    const char* src = "func sub_block_1() { var a int = 10; { b := a+1; b++; } }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;
    ruyi_ast *temp_ast4 = NULL;
    ruyi_ast *temp_ast5 = NULL;
    ruyi_ast *ast_declarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(2 == ruyi_ast_child_length(temp_ast));
    
    // var a int = 10;
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_var_declaration == temp_ast2->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast2->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_type_int == ruyi_ast_get_child(temp_ast2, 0)->type);
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 1);
    assert(Ruyi_at_integer == temp_ast3->type);
    assert(10 == temp_ast3->data.int32_value);

    // { b := a+1; b++; }
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_sub_block_statement == temp_ast2->type);
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(2 == ruyi_ast_child_length(temp_ast3));
    // b := a+1;
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_var_declaration == temp_ast4->type);
    assert(2 == ruyi_ast_child_length(temp_ast4));
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast4->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_var_declaration_auto_type == ruyi_ast_get_child(temp_ast4, 0)->type);
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 1);
    assert(Ruyi_at_additive_expression == temp_ast5->type);
    assert(3 == ruyi_ast_child_length(temp_ast5));
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast5, 0)->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast5, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    assert(Ruyi_at_op_add == ruyi_ast_get_child(temp_ast5, 1)->type);
    assert(Ruyi_at_integer == ruyi_ast_get_child(temp_ast5, 2)->type);
    assert(1 == ruyi_ast_get_child(temp_ast5, 2)->data.int32_value);
    
    ruyi_ast_destroy(ast);
}

void test_parser_function_func_type(void) {
    const char* src = "func func_type1() func(int) (int, long) { a := 10; return func(b int) (int, long) { return a+b, 20 } }";
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    ruyi_ast *func_ast;
    ruyi_unicode_string* name = NULL;
    ruyi_ast *temp_ast = NULL;
    ruyi_ast *temp_ast20 = NULL;
    ruyi_ast *temp_ast2 = NULL;
    ruyi_ast *temp_ast3 = NULL;
    ruyi_ast *temp_ast4 = NULL;
    ruyi_ast *temp_ast5 = NULL;
    ruyi_ast *temp_ast6 = NULL;
    ruyi_ast *temp_ast7 = NULL;
    ruyi_ast *ast_declarations;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    assert(Ruyi_at_root == ast->type);
    ast_declarations = ruyi_ast_get_child(ast, 2);
    assert(1 == ruyi_ast_child_length(ast_declarations));
    func_ast = ruyi_ast_get_child(ast_declarations, 0);
    assert(Ruyi_at_function_declaration == func_ast->type);
    
    // name
    temp_ast = ruyi_ast_get_child(func_ast, 0);
    assert(temp_ast != NULL);
    assert(Ruyi_at_name == temp_ast->type);
    name = ruyi_unicode_string_init_from_utf8("func_type1", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    
    // return type
    // func(int) (int,long)
    temp_ast = ruyi_ast_get_child(func_ast, 2);
    assert(temp_ast != NULL);
    assert(Ruyi_at_type_list == temp_ast->type);
    assert(1 == ruyi_ast_child_length(temp_ast));
    assert(Ruyi_at_type_func == ruyi_ast_get_child(temp_ast, 0)->type);
    // func(int) (int, long)
    temp_ast20 = ruyi_ast_get_child(temp_ast, 0);
    // func(int)
    temp_ast2 = ruyi_ast_get_child(temp_ast20, 0);
    assert(Ruyi_at_parameter_type_list == temp_ast2->type);
    assert(1 == ruyi_ast_child_length(temp_ast2));
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_type_int == temp_ast3->type);
    // (int,long)
    temp_ast2 = ruyi_ast_get_child(temp_ast20, 1);
    assert(Ruyi_at_type_list == temp_ast2->type);
    assert(2 == ruyi_ast_child_length(temp_ast20));
    assert(Ruyi_at_type_int == ruyi_ast_get_child(temp_ast2, 0)->type);
    assert(Ruyi_at_type_long == ruyi_ast_get_child(temp_ast2, 1)->type);
    
    // body
    temp_ast = ruyi_ast_get_child(func_ast, 3);
    assert(temp_ast != NULL);
    assert(Ruyi_at_block_statements == temp_ast->type);
    assert(2 == ruyi_ast_child_length(temp_ast));
    // a := 10;
    temp_ast2 = ruyi_ast_get_child(temp_ast, 0);
    assert(Ruyi_at_var_declaration == temp_ast2->type);
    // return func(b int) (int, long) { return a+b, 20 }
    temp_ast2 = ruyi_ast_get_child(temp_ast, 1);
    assert(Ruyi_at_return_statement == temp_ast2->type);
    // func(b int) (int, long) { return a+b, 20 }
    temp_ast3 = ruyi_ast_get_child(temp_ast2, 0);
    assert(Ruyi_at_anonymous_function_declaration == temp_ast3->type);
    assert(3 == ruyi_ast_child_length(temp_ast3));
    // func(b int)
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 0);
    assert(Ruyi_at_formal_parameter_list == temp_ast4->type);
    assert(1 == ruyi_ast_child_length(temp_ast4));
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_formal_parameter == temp_ast5->type);
    temp_ast6 = ruyi_ast_get_child(temp_ast5, 0);
    assert(Ruyi_at_name == temp_ast6->type);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)temp_ast6->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    temp_ast6 = ruyi_ast_get_child(temp_ast5, 1);
    assert(Ruyi_at_type_int == temp_ast6->type);
    // (int, long)
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 1);
    assert(Ruyi_at_type_list == temp_ast4->type);
    assert(2 == ruyi_ast_child_length(temp_ast4));
    assert(Ruyi_at_type_int == ruyi_ast_get_child(temp_ast4, 0)->type);
    assert(Ruyi_at_type_long == ruyi_ast_get_child(temp_ast4, 1)->type);
    // { return a+b, 20 }
    temp_ast4 = ruyi_ast_get_child(temp_ast3, 2);
    assert(Ruyi_at_block_statements == temp_ast4->type);
    assert(1 == ruyi_ast_child_length(temp_ast4));
    // return a+b, 20
    temp_ast5 = ruyi_ast_get_child(temp_ast4, 0);
    assert(Ruyi_at_return_statement == temp_ast5->type);
    assert(1 == ruyi_ast_child_length(temp_ast5));
    temp_ast6 = ruyi_ast_get_child(temp_ast5, 0);
    assert(Ruyi_at_expr_list == temp_ast6->type);
    assert(2 == ruyi_ast_child_length(temp_ast6));
    // a+b
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 0);
    assert(Ruyi_at_additive_expression == temp_ast7->type);
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast7, 0)->type);
    assert(Ruyi_at_op_add == ruyi_ast_get_child(temp_ast7, 1)->type);
    assert(Ruyi_at_name == ruyi_ast_get_child(temp_ast7, 2)->type);
    name = ruyi_unicode_string_init_from_utf8("a", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast7, 0)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    name = ruyi_unicode_string_init_from_utf8("b", 0);
    assert(ruyi_unicode_string_equals(name, (ruyi_unicode_string*)ruyi_ast_get_child(temp_ast7, 2)->data.ptr_value));
    ruyi_unicode_string_destroy(name);
    // 20
    temp_ast7 = ruyi_ast_get_child(temp_ast6, 1);
    assert(Ruyi_at_integer == temp_ast7->type);
    assert(20 == temp_ast7->data.int32_value);

    ruyi_ast_destroy(ast);
}

void test_cg_ir() {
    ruyi_ir_ins ins;
    ruyi_ir_ins_detail detail;
    assert(ruyi_ir_get_ins_code("iadd", &ins));
    assert(Ruyi_ir_Iadd == ins);
    assert(ruyi_ir_get_ins_detail(Ruyi_ir_Isub, &detail));
    assert(0 == strcmp("isub", detail.name));
    assert(detail.has_second == FALSE);
    assert(detail.may_jump == FALSE);
    assert(detail.operand == -1);
}

void test_cg_package_import_global_vars() {
    // TODO bug# 可以存在多个packages声明
    const char* src = "package bb.cc; import a1.cc\n import a2; \n c2 := 10; var c33 long = 16;";
    const char* package_name = "bb.cc";
    ruyi_cg_file_global_var *gv;
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    ruyi_cg_file *ir_file;
    err = ruyi_cg_generate(ast, &ir_file);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    ruyi_ast_destroy(ast);
    assert(0 == memcmp(package_name, ir_file->package, ir_file->package_size));
    assert(2 == ir_file->gv_count);
    
    gv = ir_file->gv[0];
    assert(0 == gv->index);
    assert(2 == gv->name_size);
    assert(0 == memcmp("c2", gv->name, gv->name_size));
    assert(Ruyi_ir_type_Int32 == gv->type);
    assert(4 == gv->var_size);
    
    gv = ir_file->gv[1];
    assert(1 == gv->index);
    assert(3 == gv->name_size);
    assert(0 == memcmp("c33", gv->name, gv->name_size));
    assert(Ruyi_ir_type_Int64 == gv->type);
    assert(8 == gv->var_size);
    
    ruyi_cg_file_destroy(ir_file);
}

void test_cg_funcs() {
    const char* src = "package bb.cc; import a2; \n c2 := 10; func f1(a1 int, a2 long) int { return a1*2 + a2; } \n"
                        "func f2(arg1 int, arg2 long) (long, int) { c := arg2; return arg1 + c, 20; }";
    ruyi_cg_file_function *func;
    ruyi_file *file = ruyi_file_init_by_data(src, (UINT32)strlen(src));
    ruyi_lexer_reader* reader = ruyi_lexer_reader_open(file);
    ruyi_error *err = NULL;
    ruyi_ast *ast = NULL;
    err = ruyi_parse_ast(reader, &ast);
    ruyi_lexer_reader_close(reader);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    ruyi_cg_file *ir_file;
    err = ruyi_cg_generate(ast, &ir_file);
    if (err != NULL) {
        printf("[error] line: %d, column:%d message: %s\n", err->line, err->column, err->message);
        ruyi_error_destroy(err);
        return;
    }
    ruyi_ast_destroy(ast);
    assert(5 == ir_file->package_size);
    assert(0 == memcmp("bb.cc", ir_file->package, ir_file->package_size));

    assert(2 == ir_file->func_count);
    // 1st function
    func = ir_file->func[0];
    // name
    assert(2 == func->name_size);
    assert(0 == strcmp("f1", (const char*)func->name));
    // return type
    assert(1 == func->return_size);
    assert(Ruyi_ir_type_Int32 == func->return_types[0]);
    // argments
    assert(2 == func->argument_size);
    assert(Ruyi_ir_type_Int32 == func->argument_types[0]);
    assert(Ruyi_ir_type_Int64 == func->argument_types[1]);
    // TODO
    
    ruyi_cg_file_destroy(ir_file);
}

void run_test_cases_basic(void) {
    test_lists();
    test_vectors();
    test_hashtable();
    test_hashtable_unicode_str();
    test_unicode();
    test_unicode_string();
    //  test_file();
    //  test_unicode_file();
}

void run_test_cases_lexer(void) {
    test_lexer_id_number();
    test_lexer_id_number_string_char_comments();
    test_lexer_symbols();
    test_lexer_keywords();
}

void run_test_cases_parser() {
    test_parser_package_import_vars();
    test_parser_expression();
    test_parser_function_return();
    test_parser_function_if();
    test_parser_function_while();
    test_parser_function_for();
    test_parser_function_switch();
    test_parser_function_break_continue();
    test_parser_function_label();
    test_parser_function_sub_block();
    test_parser_function_func_type();
}

void run_test_cases_cg() {
    test_cg_ir();
    test_cg_package_import_global_vars();
    test_cg_funcs();
}

void run_test_cases(void) {
    run_test_cases_basic();
    run_test_cases_lexer();
    run_test_cases_parser();
    run_test_cases_cg();
}
