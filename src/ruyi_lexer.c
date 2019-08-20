//
//  ruyi_lexer.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/20.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_lexer.h"
#include "ruyi_mem.h"

ruyi_lexer_reader* ruyi_lexer_reader_open(ruyi_file *file) {
    assert(file);
    ruyi_lexer_reader *reader = (ruyi_lexer_reader*)ruyi_mem_alloc(sizeof(ruyi_lexer_reader));
    reader->file = file;
    reader->token_buffer_queue = ruyi_list_create();
    reader->chars_buffer_queue = ruyi_list_create();
    reader->unicode_pool = ruyi_hashtable_create();
    return reader;
}

void ruyi_lexer_reader_close(ruyi_lexer_reader *reader) {
    assert(reader);
    ruyi_hashtable_iterator it;
    ruyi_value value;
    ruyi_list_destroy(reader->token_buffer_queue);
    ruyi_list_destroy(reader->chars_buffer_queue);
    ruyi_file_close(reader->file);
    ruyi_hashtable_iterator_get(reader->unicode_pool, &it);
    while (ruyi_hashtable_iterator_next(&it, NULL, &value)) {
        if (value.type == Ruyi_value_type_ptr) {
            if (value.data.ptr) {
                ruyi_mem_free(value.data.ptr);
            }
        }
    }
    ruyi_hashtable_destory(reader->unicode_pool);
    ruyi_mem_free(reader);
}

static BOOL ruyi_lexer_next_token_impl(ruyi_lexer_reader *reader, ruyi_token *token) {
    
    return FALSE;
}

static WIDE_CHAR ruyi_lexer_read_next_char(ruyi_lexer_reader *reader) {
    
    return 0;
}


void ruyi_lexer_push_back(ruyi_lexer_reader *reader, const ruyi_token *token) {
    assert(reader);
    assert(token);
    ruyi_token* saved_ptr;
    ruyi_value value;
    saved_ptr = (ruyi_token*) ruyi_mem_alloc(sizeof(ruyi_token));
    *saved_ptr = *token;
    value = ruyi_value_ptr(saved_ptr);
    ruyi_list_add_last(reader->token_buffer_queue, value);
}

BOOL ruyi_lexer_next_token(ruyi_lexer_reader *reader, ruyi_token *token) {
    assert(reader);
    assert(token);
    ruyi_value value;
    ruyi_token *saved_ptr;
    if (!ruyi_list_empty(reader->token_buffer_queue)) {
        ruyi_list_remove_first(reader->token_buffer_queue, &value);
        saved_ptr = (ruyi_token*)value.data.ptr;
        if (saved_ptr) {
            *token = *saved_ptr;
            ruyi_mem_free(saved_ptr);
        } else {
            return FALSE;
        }
    }
    return ruyi_lexer_next_token_impl(reader, token);
}

UINT32 ruyi_lexer_get_token_text(const ruyi_lexer_reader *reader, const ruyi_token *token, WIDE_CHAR* out_buf, UINT32 buf_length) {
    assert(reader);
    assert(token);
    assert(out_buf);
    ruyi_value key, value;
    if (token->type == Ruyi_tt_STRING || token->type == Ruyi_tt_IDENTITY) {
        key = ruyi_value_int64(token->value.int_value);
        if (!ruyi_hashtable_get(reader->unicode_pool, key, &value)) {
            return 0;
        }
        return ruyi_value_get_unicode_str(value, out_buf, buf_length);
    }
    return 0;
}

