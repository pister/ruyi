//
//  ruyi_lexer.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/20.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_lexer.h"
#include "ruyi_mem.h"
#include "ruyi_unicode.h"

typedef struct {
    UINT32 line;
    UINT32 column;
    WIDE_CHAR c;
} ruyi_pos_char;

ruyi_lexer_reader* ruyi_lexer_reader_open(ruyi_file *file) {
    assert(file);
    ruyi_lexer_reader *reader = (ruyi_lexer_reader*)ruyi_mem_alloc(sizeof(ruyi_lexer_reader));
    reader->file = ruyi_io_unicode_file_open(file);
    reader->token_buffer_queue = ruyi_list_create();
    reader->chars_buffer_queue = ruyi_list_create();
    reader->line = 1;
    reader->column = 1;
    return reader;
}

void ruyi_lexer_reader_close(ruyi_lexer_reader *reader) {
    assert(reader);
    ruyi_list_destroy(reader->token_buffer_queue);
    ruyi_list_destroy(reader->chars_buffer_queue);
    ruyi_io_unicode_file_close(reader->file);
    ruyi_mem_free(reader);
}

static UINT64 ruyi_make_pos_value(UINT32 line, UINT32 column) {
    UINT64 v = line;
    v = (v & 0x00000000FFFFFFFF) << 32;
    v |= (column & 0x00000000FFFFFFFF);
    return v;
}

static void ruyi_lexer_reader_push_back_char(ruyi_lexer_reader *reader, ruyi_pos_char pc) {
    UINT64 v = ruyi_make_pos_value(pc.line, pc.column);
    ruyi_list_add_first(reader->chars_buffer_queue, ruyi_value_uint64(v));
    ruyi_list_add_first(reader->chars_buffer_queue, ruyi_value_int32(pc.c));
}

static BOOL ruyi_lexer_read_next_char(ruyi_lexer_reader *reader, ruyi_pos_char *pos_char) {
    ruyi_value temp;
    WIDE_CHAR buffer[256];
    UINT32 read_length;
    UINT32 i;
    UINT64 v;
    for (;;) {
        if (!ruyi_list_empty(reader->chars_buffer_queue)) {
            ruyi_list_remove_first(reader->chars_buffer_queue, &temp);
            pos_char->c = temp.data.int32_value;
            ruyi_list_remove_first(reader->chars_buffer_queue, &temp);
            v = temp.data.uint64_value;
            pos_char->line = (v & 0xFFFFFFFF00000000) >> 32;
            pos_char->column = (v & 0x00000000FFFFFFFF);
            return TRUE;
        }
        read_length = ruyi_io_unicode_file_read_utf8(reader->file, buffer, 256);
        if (read_length == 0) {
            pos_char->c = 0;
            return FALSE;
        }
        for (i = 0; i < read_length; i++) {
            ruyi_list_add_last(reader->chars_buffer_queue, ruyi_value_int32(buffer[i]));
            v = ruyi_make_pos_value(reader->line, reader->column);
            ruyi_list_add_last(reader->chars_buffer_queue, ruyi_value_uint64(v));
            if (buffer[i] == '\n') {
                reader->line++;
                reader->column = 1;
            } else {
                if (buffer[i] != '\r') {
                    reader->column++;
                }
            }
        }
    }
}

static BOOL ruyi_lexer_peek_char(ruyi_lexer_reader *reader, ruyi_pos_char *pos_char)  {
    BOOL ret = ruyi_lexer_read_next_char(reader, pos_char);
    ruyi_lexer_reader_push_back_char(reader, *pos_char);
    return ret;
}

#define RUYI_IS_LETTER(c) \
((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))

#define RUYI_IS_DIGIT(c) \
((c >= '0' && c <= '9'))

#define RUYI_IS_SPACE(c) \
((c == ' ' || c == '\t' || c == '\r'))

static ruyi_token* ruyi_lexer_make_token(ruyi_token_type token_type, ruyi_pos_char first) {
    ruyi_token* token = (ruyi_token*)ruyi_mem_alloc(sizeof(ruyi_token));
    token->type = token_type;
    token->line = first.line;
    token->column = first.column;
    return token;
}

static ruyi_token * ruyi_lexer_handle_number(ruyi_lexer_reader *reader, ruyi_pos_char pc) {
    assert(reader);
    ruyi_pos_char ch;
    BOOL minus = FALSE;
    if ('-' == pc.c) {
        if (!ruyi_lexer_peek_char(reader, &ch)) {
            return NULL;
        }
        if (!RUYI_IS_DIGIT(ch.c) && ch.c != '.') {
            return ruyi_lexer_make_token(Ruyi_tt_SYMBOL_SUB, pc);
        }
        minus = TRUE;
        // read first char
        if (!ruyi_lexer_read_next_char(reader, &ch)) {
            return NULL;
        }
    }
    if ('0' == pc.c) {
        
    } else {
        
    }
    
    return NULL;
}

static ruyi_token * ruyi_lexer_handle_identifier(ruyi_lexer_reader *reader, ruyi_pos_char pc) {
    assert(reader);
    ruyi_unicode_string* string_value = ruyi_unicode_string_init_with_capacity(64);
    ruyi_pos_char first = pc;
    ruyi_pos_char pos_char;
    WIDE_CHAR c;
    ruyi_token* token;
    do {
        ruyi_unicode_string_append_wide_char(string_value, first.c);
        for(;;) {
            if (!ruyi_lexer_read_next_char(reader, &pos_char)) {
                break;
            }
            c = pos_char.c;
            if (RUYI_IS_LETTER(c) || c == '_' || RUYI_IS_DIGIT(c)) {
                ruyi_unicode_string_append_wide_char(string_value, c);
            } else {
                ruyi_lexer_reader_push_back_char(reader, pos_char);
                break;
            }
        }
    } while (FALSE);
    token = ruyi_lexer_make_token(Ruyi_tt_IDENTITY, first);
    token->value.str_value = string_value;
    token->size = string_value->length;
    return token;
}



static ruyi_token* ruyi_lexer_next_token_impl(ruyi_lexer_reader *reader) {
    ruyi_pos_char pc;
    WIDE_CHAR c;
    for (;;) {
        if (!ruyi_lexer_read_next_char(reader, &pc)) {
            return ruyi_lexer_make_token(Ruyi_tt_END, pc);
        }
        c = pc.c;
        if (RUYI_IS_SPACE(c)) {
            continue;
        }
        if (RUYI_IS_LETTER(c) || c == '_') {
            return ruyi_lexer_handle_identifier(reader, pc);
        }
        if (RUYI_IS_DIGIT(c)) {
            return ruyi_lexer_handle_number(reader, pc);
        }
        switch (c) {
            case '\n':
                return ruyi_lexer_make_token(Ruyi_tt_EOL, pc);
                
            default:
                break;
        }
    }
    return NULL;
}

void ruyi_lexer_reader_push_back(ruyi_lexer_reader *reader, ruyi_token *token) {
    assert(reader);
    assert(token);
    ruyi_list_add_first(reader->token_buffer_queue, ruyi_value_ptr(token));
}

ruyi_token* ruyi_lexer_reader_next_token(ruyi_lexer_reader *reader) {
    assert(reader);
    ruyi_value value;
    if (!ruyi_list_empty(reader->token_buffer_queue)) {
        ruyi_list_remove_first(reader->token_buffer_queue, &value);
        return (ruyi_token*)value.data.ptr;
    }
    return ruyi_lexer_next_token_impl(reader);
}

void ruyi_lexer_reader_consume_token(ruyi_lexer_reader *reader) {
    ruyi_token * token = ruyi_lexer_reader_next_token(reader);
    if (token) {
        ruyi_lexer_token_destroy(token);
    }
}

void ruyi_lexer_token_destroy(ruyi_token * token) {
    assert(token);
    switch (token->type) {
        case Ruyi_tt_IDENTITY:
        case Ruyi_tt_STRING:
            if (token->value.str_value) {
                ruyi_unicode_string_destroy(token->value.str_value);
            }
            break;
        default:
            break;
    }
    ruyi_mem_free(token);
}
