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
#include "ruyi_hashtable.h"
#include <string.h> // for memcpy

typedef struct {
    ruyi_token_type type;
    char keyword[64];
} ruyi_keyword;

ruyi_keyword g_ruyi_keywords[] = {
    {Ruyi_tt_KW_IF,     "if"},
    {Ruyi_tt_KW_ELSE,   "else"},
    {Ruyi_tt_KW_ELSEIF, "elseif"},
    {Ruyi_tt_KW_WHILE,  "while"},
    {Ruyi_tt_KW_FOR,    "for"},
    {Ruyi_tt_SWITCH,    "switch"},
    {Ruyi_tt_THIS,      "this"},
    {Ruyi_tt_KW_RETURN, "return"},
    {Ruyi_tt_KW_BREAK,  "break"},
    {Ruyi_tt_KW_CASE,   "case"},
    {Ruyi_tt_KW_CONTINUE,"continue"},
    {Ruyi_tt_KW_DEFAULT,"default"},
    {Ruyi_tt_KW_FUNC,   "func"},
    {Ruyi_tt_KW_CLASS,  "class"},
    {Ruyi_tt_KW_NEW,    "new"},
    {Ruyi_tt_KW_VAR,    "var"},
    {Ruyi_tt_KW_BYTE,   "byte"},
    {Ruyi_tt_KW_BOOL,   "bool"},
    {Ruyi_tt_KW_INT,    "int"},
    {Ruyi_tt_KW_LONG,   "long"},
    {Ruyi_tt_KW_SHORT,  "short"},
    {Ruyi_tt_KW_FLOAT,  "float"},
    {Ruyi_tt_KW_RUNE,   "rune"},
    {Ruyi_tt_KW_TRY,    "try"},
    {Ruyi_tt_KW_CATCH,  "catch"},
    {Ruyi_tt_KW_THROW,  "throw"},
    {Ruyi_tt_KW_STATIC, "static"},
    {Ruyi_tt_KW_ENUM,   "enum"},
    {Ruyi_tt_KW_DO,     "do"},
    {Ruyi_tt_KW_GOTO,   "goto"},
    {Ruyi_tt_KW_CHAR,   "char"},
    {Ruyi_tt_KW_CONST,  "const"},
    {Ruyi_tt_KW_DOUBLE, "double"},
    {Ruyi_tt_KW_PACKAGE,"package"},
    {Ruyi_tt_KW_IMPORT, "import"},
    {Ruyi_tt_KW_TRUE,   "true"},
    {Ruyi_tt_KW_FALSE,  "false"},
    {Ruyi_tt_KW_NULL,   "null"},
    {Ruyi_tt_KW_ARRAY,  "array"},
    {Ruyi_tt_KW_MAP,    "map"},
    {Ruyi_tt_KW_THIS,   "this"},
    {Ruyi_tt_KW_INSTANCEOF, "instanceof"},
};


ruyi_token_type ruyi_lexer_keywords_get_type(ruyi_unicode_string * token_value) {
    static ruyi_hashtable* keywords;
    UINT32 i;
    ruyi_value ret_type;
    if (keywords == NULL) {
        keywords = ruyi_hashtable_create();
        for (i = 0; i < sizeof(g_ruyi_keywords)/sizeof(ruyi_keyword); i++) {
            ruyi_hashtable_put(keywords, ruyi_value_unicode_str(ruyi_unicode_string_init_from_utf8(g_ruyi_keywords[i].keyword, 0)), ruyi_value_int64(g_ruyi_keywords[i].type));
        }
    }
    if (ruyi_hashtable_get(keywords, ruyi_value_unicode_str(token_value), &ret_type)) {
        return (ruyi_token_type)ret_type.data.int64_value;
    }
    return Ruyi_tt_IDENTITY;
}

ruyi_unicode_string* ruyi_lexer_keywords_get_str(ruyi_token_type type) {
    static ruyi_hashtable* keywords;
    UINT32 i;
    ruyi_value ret_str;
    if (keywords == NULL) {
        keywords = ruyi_hashtable_create();
        for (i = 0; i < sizeof(g_ruyi_keywords)/sizeof(ruyi_keyword); i++) {
            ruyi_hashtable_put(keywords, ruyi_value_int64(g_ruyi_keywords[i].type), ruyi_value_unicode_str(ruyi_unicode_string_init_from_utf8(g_ruyi_keywords[i].keyword, 0)));
        }
    }
    if (ruyi_hashtable_get(keywords, ruyi_value_int64(type), &ret_str)) {
        return ret_str.data.unicode_str;
    }
    return NULL;
}

typedef struct {
    UINT32 line;
    UINT32 column;
    WIDE_CHAR c;
} ruyi_pos_char;

typedef enum {
    Ruyi_r_OK,
    Ruyi_r_END,
    Ruyi_r_ERROR
} ruyi_lexer_result;

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

static void ruyi_lexer_error_message(const char* msg, ruyi_pos_char first) {
    printf("lexer error: %s at line: %d, column: %d\n", msg, first.line, first.column);
}

static void ruyi_lexer_copy_last_n_chars(WIDE_CHAR *dest, UINT32 dest_size, ruyi_unicode_string* src) {
    UINT32 size = ruyi_unicode_string_length(src);
    UINT32 copy_pos, copy_len;
    if (size < dest_size - 1) {
        copy_pos = 0;
        copy_len = size;
    } else {
        copy_len = dest_size - 1;
        copy_pos = size - dest_size + 1;
    }
    memcpy(dest, src->data + copy_pos, copy_len * sizeof(WIDE_CHAR));
    dest[copy_len] = '\0';
}

static void ruyi_lexer_set_snapshot(ruyi_token_snapshot *token_snapshot, ruyi_token *token) {
    ruyi_unicode_string* keyword;
    if (token == NULL) {
        token_snapshot->column = 0;
        token_snapshot->line = 0;
        token_snapshot->size = 0;
        token_snapshot->value.int_value = 0;
        token_snapshot->str_snapshot[0] = '\0';
    } else {
        token_snapshot->column = token->column;
        token_snapshot->line = token->line;
        token_snapshot->size = token->size;
        keyword = ruyi_lexer_keywords_get_str(token->type);
        if (keyword) {
            ruyi_lexer_copy_last_n_chars(token_snapshot->str_snapshot, LEXER_TOKEN_SNAPSHOT_STR_SIZE, keyword);
        } else {
            switch (token->type) {
                case Ruyi_tt_IDENTITY:
                case Ruyi_tt_STRING:
                    ruyi_lexer_copy_last_n_chars(token_snapshot->str_snapshot, LEXER_TOKEN_SNAPSHOT_STR_SIZE, token->value.str_value);
                    break;
                default:
                    break;
            }
        }
    }
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

static void ruyi_lexer_reader_push_front_char(ruyi_lexer_reader *reader, ruyi_pos_char pc) {
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
    ruyi_lexer_reader_push_front_char(reader, *pos_char);
    return ret;
}

#define RUYI_IS_LETTER(c) \
((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))

#define RUYI_IS_DIGIT(c) \
((c >= '0' && c <= '9'))

#define RUYI_IS_SPACE(c) \
((c == ' ' || c == '\t' || c == '\r' || c == '\n'))

static ruyi_token* ruyi_lexer_make_token_with_size(ruyi_token_type token_type, ruyi_pos_char first, UINT32 size) {
    ruyi_token* token = (ruyi_token*)ruyi_mem_alloc(sizeof(ruyi_token));
    token->type = token_type;
    token->line = first.line;
    token->column = first.column;
    token->size = size;
    return token;
}

static ruyi_token* ruyi_lexer_make_token(ruyi_token_type token_type, ruyi_pos_char first) {
    return ruyi_lexer_make_token_with_size(token_type, first, 1);
}

static ruyi_token * ruyi_lexer_make_number_token(INT64 integer_part, double fraction_part, INT32 exponent_part, BOOL has_dot, ruyi_pos_char first, UINT32 size) {
    ruyi_token * token = ruyi_lexer_make_token(has_dot ? Ruyi_tt_FLOAT : Ruyi_tt_INTEGER, first);
    double exponent = 1;
    while (exponent_part != 0) {
        if (exponent_part > 0) {
            exponent *= 10;
            exponent_part--;
        } else if (exponent_part < 0) {
            exponent *= 0.1;
            exponent_part++;
        }
    }
    if (!has_dot) {
        token->value.int_value = integer_part;
        token->value.int_value *= exponent;
    } else {
        token->value.float_value = integer_part + fraction_part;
        token->value.float_value *= exponent;
    }
    token->size = size;
    return token;
}

static ruyi_lexer_result ruyi_lexer_get_digit(ruyi_pos_char first, ruyi_pos_char ch, INT64 radix, int *out) {
    switch (radix) {
        case 2:
            if (ch.c == '0' || ch.c == '1') {
                *out = ch.c - '0';
                return Ruyi_r_OK;
            } else if (ch.c >= '2' && ch.c <= '9') {
                ruyi_lexer_error_message("binary number only support digit 0 and 1", first);
                return Ruyi_r_ERROR;
            } else {
                return Ruyi_r_END;
            }
        case 8:
            if (ch.c >= '0' && ch.c <= '7') {
                *out = ch.c - '0';
                return Ruyi_r_OK;
            } else if (ch.c >= '8' && ch.c <= '9') {
                ruyi_lexer_error_message("octal number only support digit 0 to 7", first);
                return Ruyi_r_ERROR;
            } else {
                return Ruyi_r_END;
            }
        case 10:
            if (ch.c >= '0' && ch.c <= '9') {
                *out = ch.c - '0';
                return Ruyi_r_OK;
            } else {
                return Ruyi_r_END;
            }
        case 16:
            if (ch.c >= '0' && ch.c <= '9') {
                *out = ch.c - '0';
                return Ruyi_r_OK;
            } else if (ch.c >= 'A' && ch.c <= 'F') {
                *out = ch.c - 'A' + 10;
                return Ruyi_r_OK;
            } else if (ch.c >= 'a' && ch.c <= 'f') {
                *out = ch.c - 'a' + 10;
                return Ruyi_r_OK;
            } else {
                return Ruyi_r_END;
            }
        default:
            ruyi_lexer_error_message("unsupport number radix", first);
            return Ruyi_r_ERROR;
    }
}

static ruyi_token * ruyi_lexer_get_decimal(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_pos_char ch = first;
    INT64 integer_part = 0;
    double fraction_part = 0;
    double fraction_base = 0.1;
    UINT32 exponent_part = 0;
    INT64 radix = 10;
    UINT32 size = 0;
    BOOL has_dot = FALSE;
    // 10-base
    integer_part = radix * integer_part + first.c - '0';
    for (;;) {
        if (!ruyi_lexer_read_next_char(reader, &ch)) {
            ruyi_lexer_reader_push_front_char(reader, ch);
            return ruyi_lexer_make_number_token(integer_part, fraction_part, exponent_part, has_dot, first, size);
        }
        size++;
        if (RUYI_IS_DIGIT(ch.c)) {
            if (has_dot) {
                fraction_part = fraction_part + (ch.c - '0') * fraction_base;
                fraction_base *= 0.1;
            } else {
                integer_part = radix * integer_part + ch.c - '0';
            }
        } else if ('.' == ch.c) {
            if (has_dot) {
                ruyi_lexer_reader_push_front_char(reader, ch);
                return ruyi_lexer_make_number_token(integer_part, fraction_part, exponent_part, TRUE, first, size);
            } else {
                has_dot = TRUE;
            }
        } else if ('e' == ch.c || 'E' == ch.c) {
            // exp part
            ruyi_lexer_peek_char(reader, &ch);
            if (RUYI_IS_DIGIT(ch.c) || '-' == ch.c) {
                if ('-' == ch.c) {
                    ruyi_lexer_read_next_char(reader, &ch);
                    exponent_part = -1;
                    ruyi_lexer_peek_char(reader, &ch);
                    if (!RUYI_IS_DIGIT(ch.c)) {
                        ruyi_lexer_error_message("exponent miss digit", first);
                    }
                }
                for (;;) {
                    if (!ruyi_lexer_read_next_char(reader, &ch)) {
                        ruyi_lexer_reader_push_front_char(reader, ch);
                        return ruyi_lexer_make_number_token(integer_part, fraction_part, exponent_part, has_dot, first, size);
                    }
                    if (!RUYI_IS_DIGIT(ch.c)) {
                        ruyi_lexer_reader_push_front_char(reader, ch);
                        return ruyi_lexer_make_number_token(integer_part, fraction_part, exponent_part, has_dot, first, size);
                    }
                    size++;
                    exponent_part = 10 * exponent_part + ch.c - '0';
                }
            } else {
                ruyi_lexer_error_message("exponent miss digit", first);
                return NULL;
            }
        } else {
            ruyi_lexer_reader_push_front_char(reader, ch);
            return ruyi_lexer_make_number_token(integer_part, fraction_part, exponent_part, has_dot, first, size);
        }
    }
}

static ruyi_token * ruyi_lexer_handle_dot3(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_pos_char ch1 = first;
    ruyi_pos_char ch2 = first;
    if (!ruyi_lexer_read_next_char(reader, &ch1)) {
        return ruyi_lexer_make_token(Ruyi_tt_DOT, first);
    }
    if ('.' != ch1.c) {
        ruyi_lexer_reader_push_front_char(reader, ch1);
        return ruyi_lexer_make_token(Ruyi_tt_DOT, first);
    }
    if (!ruyi_lexer_peek_char(reader, &ch2)) {
        ruyi_lexer_reader_push_front_char(reader, ch1);
        return ruyi_lexer_make_token(Ruyi_tt_DOT, first);
    }
    if ('.' != ch2.c) {
        ruyi_lexer_reader_push_front_char(reader, ch1);
        return ruyi_lexer_make_token(Ruyi_tt_DOT, first);
    }
    ruyi_lexer_read_next_char(reader, &ch2);
    return ruyi_lexer_make_token(Ruyi_tt_DOT3, first);
}

static ruyi_token * ruyi_lexer_handle_number(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    assert(reader);
    ruyi_pos_char ch = first;
    INT64 integer_part = 0;
    INT64 radix = 10;
    UINT32 size = 0;
    ruyi_lexer_result lr;
    int out;
    if ('.' == first.c) {
        if (!ruyi_lexer_peek_char(reader, &ch)) {
            return ruyi_lexer_make_token(Ruyi_tt_DOT, first);
        }
        if (!RUYI_IS_DIGIT(ch.c)) {
            // Ruyi_tt_DOT3
            return ruyi_lexer_handle_dot3(reader, first);
           // return ruyi_lexer_make_token(Ruyi_tt_DOT, first);
        }
        ch.c = '0';
        ruyi_lexer_reader_push_front_char(reader, first);
        return ruyi_lexer_get_decimal(reader, ch);
    } else if (first.c == '0') {
        if (!ruyi_lexer_read_next_char(reader, &ch)) {
            return ruyi_lexer_make_number_token(integer_part, 0, 0, FALSE, first, size);
        }
        size ++;
        if (ch.c == 'x' || ch.c == 'X') {
            radix = 16;
            
        } else if ( ch.c >= '0' && ch.c <= '7') {
            radix = 8;
            integer_part = integer_part * radix + ch.c - '0';
        } else if ( ch.c >= '8' && ch.c <= '9') {
            ruyi_lexer_error_message("unsupport number, the decimal must start with none-zero", first);
            return NULL;
        } else if ( ch.c == 'b' || ch.c == 'B') {
            radix = 2;
        } else if (ch.c == '.') {
            ruyi_lexer_reader_push_front_char(reader, ch);
            return ruyi_lexer_get_decimal(reader, first);
        } else {
            // just 0
            ruyi_lexer_reader_push_front_char(reader, ch);
            return ruyi_lexer_make_number_token(0, 0, 0, FALSE, first, size);
        }
        for (;;) {
            if (!ruyi_lexer_read_next_char(reader, &ch)) {
                return ruyi_lexer_make_number_token(integer_part, 0, 0, FALSE, first, size);
            }
            size++;
            if ('.' == ch.c) {
                ruyi_lexer_error_message("unsupport float number for explicit radix number", first);
                return NULL;
            }
            lr = ruyi_lexer_get_digit(first, ch, radix, &out);
            if (Ruyi_r_ERROR == lr) {
                return NULL;
            } else if (Ruyi_r_END == lr) {
                return ruyi_lexer_make_number_token(integer_part, 0, 0, FALSE, first, size);
            } else {
                integer_part = radix * integer_part + out;
            }
        }
    } else {
        // 10-base
        return ruyi_lexer_get_decimal(reader, first);
    }
    return NULL;
}

static ruyi_token * ruyi_lexer_handle_identifier(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_unicode_string* string_value = ruyi_unicode_string_init_with_capacity(64);
    ruyi_pos_char ch = first;
    WIDE_CHAR c;
    ruyi_token* token;
    ruyi_token_type type;
    ruyi_unicode_string_append_wide_char(string_value, ch.c);
    for(;;) {
        if (!ruyi_lexer_read_next_char(reader, &ch)) {
            break;
        }
        c = ch.c;
        if (RUYI_IS_LETTER(c) || c == '_' || RUYI_IS_DIGIT(c)) {
            ruyi_unicode_string_append_wide_char(string_value, c);
        } else {
            ruyi_lexer_reader_push_front_char(reader, ch);
            break;
        }
    }
    
    type = ruyi_lexer_keywords_get_type(string_value);
    if (type == Ruyi_tt_IDENTITY) {
        token = ruyi_lexer_make_token(Ruyi_tt_IDENTITY, first);
        token->value.str_value = string_value;
        token->size = string_value->length;
    } else {
        token = ruyi_lexer_make_token(type, first);
        
        ruyi_unicode_string_destroy(string_value);
    }
    return token;
}

static ruyi_token * ruyi_lexer_handle_string(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_unicode_string* string_value = ruyi_unicode_string_init_with_capacity(64);
    ruyi_pos_char ch;
    WIDE_CHAR c;
    ruyi_token* token;
    UINT32 size = 1;
    for(;;) {
        if (!ruyi_lexer_read_next_char(reader, &ch)) {
            ruyi_lexer_error_message("miss end \" for string", first);
            return NULL;
        }
        size++;
        c = ch.c;
        if (c == '\\') {
            if (!ruyi_lexer_read_next_char(reader, &ch)) {
                ruyi_lexer_error_message("miss end \" for string", first);
                return NULL;
            }
            size++;
            switch (ch.c) {
                case 'n':
                    ruyi_unicode_string_append_wide_char(string_value, '\n');
                    break;
                case '"':
                    ruyi_unicode_string_append_wide_char(string_value, '\"');
                    break;
                case 't':
                    ruyi_unicode_string_append_wide_char(string_value, '\t');
                    break;
                case 'r':
                    ruyi_unicode_string_append_wide_char(string_value, '\r');
                    break;
                case 'b':
                    ruyi_unicode_string_append_wide_char(string_value, '\b');
                    break;
                case '\\':
                    ruyi_unicode_string_append_wide_char(string_value, '\\');
                    break;
                case '\'':
                    ruyi_unicode_string_append_wide_char(string_value, '\'');
                    break;
                default:
                    ruyi_unicode_string_append_wide_char(string_value, ch.c);
                    break;
            }
        } else if (c == '\"') {
            token = ruyi_lexer_make_token(Ruyi_tt_STRING, first);
            token->value.str_value = string_value;
            token->size = size;
            return token;
        } else {
            ruyi_unicode_string_append_wide_char(string_value, c);
        }
    }
    return NULL;
}

static ruyi_token * ruyi_lexer_handle_char(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_pos_char ch;
    ruyi_token* token;
    UINT32 size = 1;
    WIDE_CHAR content;
    if (!ruyi_lexer_read_next_char(reader, &ch)) {
        ruyi_lexer_error_message("miss content for char", first);
        return NULL;
    }
    size++;
    if (ch.c == '\'') {
        ruyi_lexer_error_message("miss content for char", first);
        return NULL;
    }
    if (ch.c == '\\') {
        if (!ruyi_lexer_read_next_char(reader, &ch)) {
            ruyi_lexer_error_message("miss content for char", first);
            return NULL;
        }
        size++;
        switch (ch.c) {
            case 'n':
                content = '\n';
                break;
            case '"':
                content = '\"';
                break;
            case 't':
                content = '\t';
                break;
            case 'r':
                content = '\r';
                break;
            case 'b':
                content = '\b';
                break;
            case '\\':
                content = '\\';
                break;
            case '\'':
                content = '\'';
                break;
            default:
                content = ch.c;
                break;
        }
    } else {
        content = ch.c;
    }
    if (!ruyi_lexer_read_next_char(reader, &ch)) {
        ruyi_lexer_error_message("miss content for char", first);
        return NULL;
    }
    size++;
    if ('\'' != ch.c) {
        ruyi_lexer_error_message("need \' for end of char", first);
        return NULL;
    }
    token = ruyi_lexer_make_token(Ruyi_tt_CHAR, first);
    token->value.int_value = content;
    token->size = size;
    return token;
}

typedef struct {
    WIDE_CHAR c;
    ruyi_token_type token_type;
} ruyi_lexer_choose_tt;

static ruyi_token * ruyi_lexer_handle_mul_tokens(ruyi_lexer_reader *reader, ruyi_pos_char first, ruyi_lexer_choose_tt* c_tt, UINT32 c_tt_size, ruyi_token_type not_match) {
    ruyi_pos_char pc;
    UINT32 i;
    ruyi_token* token;
    if (!ruyi_lexer_read_next_char(reader, &pc)) {
        ruyi_lexer_reader_push_front_char(reader, pc);
        return ruyi_lexer_make_token(not_match, first);
    }
    for (i = 0; i < c_tt_size; i++) {
        if (c_tt[i].c == pc.c) {
            token = ruyi_lexer_make_token(c_tt[i].token_type, first);
            token->size = 2;
            return token;
        }
    }
    ruyi_lexer_reader_push_front_char(reader, pc);
    return ruyi_lexer_make_token(not_match, first);
}


static ruyi_token * ruyi_lexer_handle_div_or_comments(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_pos_char ch;
    ruyi_token* token;
    UINT32 size = 1;
    ruyi_lexer_choose_tt choose_tt[1];
    if (!ruyi_lexer_read_next_char(reader, &ch)) {
        ruyi_lexer_reader_push_front_char(reader, ch);
        return ruyi_lexer_make_token(Ruyi_tt_DIV, first);
    }
    size++;
    if ('/' == ch.c) {
        token = ruyi_lexer_make_token(Ruyi_tt_LINE_COMMENTS, first);
        for (;;) {
            if (!ruyi_lexer_read_next_char(reader, &ch)) {
                ruyi_lexer_reader_push_front_char(reader, ch);
                token->size = size;
                return token;
            }
            size++;
            if ('\n' == ch.c) {
                token->size = size;
                return token;
            }
        }
    } else if ('*' == ch.c) {
        token = ruyi_lexer_make_token(Ruyi_tt_MLINES_COMMENTS, first);
        for (;;) {
            if (!ruyi_lexer_read_next_char(reader, &ch)) {
                ruyi_lexer_reader_push_front_char(reader, ch);
                token->size = size;
                return token;
            }
            size ++;
            if ('*' == ch.c) {
                if (!ruyi_lexer_read_next_char(reader, &ch)) {
                    ruyi_lexer_reader_push_front_char(reader, ch);
                    token->size = size;
                    return token;
                }
                if ('/' == ch.c) {
                    size ++;
                    token->size = size;
                    return token;
                } else {
                    ruyi_lexer_reader_push_front_char(reader, ch);
                }
            }
        }
    } else {
        ruyi_lexer_reader_push_front_char(reader, ch);
        choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_DIV_ASS;
        return ruyi_lexer_handle_mul_tokens(reader, first, choose_tt, 1, Ruyi_tt_DIV);
    }
    return NULL;
}

static ruyi_token* ruyi_lexer_token_lt(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_pos_char pc;
    UINT32 size = 1;
    if (!ruyi_lexer_read_next_char(reader, &pc)) {
        ruyi_lexer_reader_push_front_char(reader, pc);
        return ruyi_lexer_make_token(Ruyi_tt_LT, first);
    }
    size++;
    switch (pc.c) {
        case '=':
            return ruyi_lexer_make_token_with_size(Ruyi_tt_LTE, first, size);
        case '<':
            if (!ruyi_lexer_peek_char(reader, &pc)) {
                return ruyi_lexer_make_token_with_size(Ruyi_tt_SHFT_LEFT, first, size);
            } else if ('=' == pc.c) {
                ruyi_lexer_read_next_char(reader, &pc);
                size++;
                return ruyi_lexer_make_token_with_size(Ruyi_tt_SHFT_LEFT_ASS, first, size);
            } else {
                return ruyi_lexer_make_token_with_size(Ruyi_tt_SHFT_LEFT, first, size);
            }
        default:
            ruyi_lexer_reader_push_front_char(reader, pc);
            return ruyi_lexer_make_token(Ruyi_tt_LT, first);
    }
    return NULL;
}

static ruyi_token* ruyi_lexer_token_gt(ruyi_lexer_reader *reader, ruyi_pos_char first) {
    ruyi_pos_char pc;
    UINT32 size = 1;
    if (!ruyi_lexer_read_next_char(reader, &pc)) {
        ruyi_lexer_reader_push_front_char(reader, pc);
        return ruyi_lexer_make_token(Ruyi_tt_GT, first);
    }
    size++;
    switch (pc.c) {
        case '=':
            return ruyi_lexer_make_token_with_size(Ruyi_tt_GTE, first, size);
        case '>':
            if (!ruyi_lexer_peek_char(reader, &pc)) {
                return ruyi_lexer_make_token_with_size(Ruyi_tt_SHFT_RIGHT, first, size);
            } else if ('=' == pc.c) {
                ruyi_lexer_read_next_char(reader, &pc);
                size++;
                return ruyi_lexer_make_token_with_size(Ruyi_tt_SHFT_RIGHT_ASS, first, size);
            } else {
                return ruyi_lexer_make_token_with_size(Ruyi_tt_SHFT_RIGHT, first, size);
            }
        default:
            ruyi_lexer_reader_push_front_char(reader, pc);
            return ruyi_lexer_make_token(Ruyi_tt_GT, first);
    }
    return NULL;
}

static ruyi_token* ruyi_lexer_next_token_impl(ruyi_lexer_reader *reader) {
    ruyi_pos_char pc;
    WIDE_CHAR c;
    ruyi_lexer_choose_tt choose_tt[4];
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
            case '.':
                return ruyi_lexer_handle_number(reader, pc);
            case '"':
                return ruyi_lexer_handle_string(reader, pc);
            case '\'':
                return ruyi_lexer_handle_char(reader, pc);
            case '+':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_ADD_ASS;
                choose_tt[1].c = '+'; choose_tt[1].token_type = Ruyi_tt_INC;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 2, Ruyi_tt_ADD);
            case '-':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_SUB_ASS;
                choose_tt[1].c = '-'; choose_tt[1].token_type = Ruyi_tt_DEC;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 2, Ruyi_tt_SUB);
            case '*':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_MUL_ASS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 1, Ruyi_tt_MUL);
            case '/':
                return ruyi_lexer_handle_div_or_comments(reader, pc);
            case '%':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_MOD_ASS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 1, Ruyi_tt_MOD);
            case '(':
                return ruyi_lexer_make_token(Ruyi_tt_LPAREN, pc);
            case ')':
                return ruyi_lexer_make_token(Ruyi_tt_RPAREN, pc);
            case '[':
                return ruyi_lexer_make_token(Ruyi_tt_LBRACKET, pc);
            case ']':
                return ruyi_lexer_make_token(Ruyi_tt_RBRACKET, pc);
            case '{':
                return ruyi_lexer_make_token(Ruyi_tt_LBRACE, pc);
            case '}':
                return ruyi_lexer_make_token(Ruyi_tt_RBRACE, pc);
            case ',':
                return ruyi_lexer_make_token(Ruyi_tt_COMMA, pc);
            case ';':
                return ruyi_lexer_make_token(Ruyi_tt_SEMICOLON, pc);
            case ':':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_COLON_ASSIGN;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 1, Ruyi_tt_COLON);
            case '<':
                return ruyi_lexer_token_lt(reader, pc);
            case '>':
                return ruyi_lexer_token_gt(reader, pc);
            case '=':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_EQUALS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 1, Ruyi_tt_ASSIGN);
            case '!':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_NOT_EQUALS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 1, Ruyi_tt_LOGIC_NOT);
            case '&':
                choose_tt[0].c = '&'; choose_tt[0].token_type = Ruyi_tt_LOGIC_AND;
                choose_tt[1].c = '='; choose_tt[1].token_type = Ruyi_tt_BIT_AND_ASS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 2, Ruyi_tt_BIT_AND);
            case '|':
                choose_tt[0].c = '|'; choose_tt[0].token_type = Ruyi_tt_LOGIC_OR;
                choose_tt[1].c = '='; choose_tt[1].token_type = Ruyi_tt_BIT_OR_ASS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 2, Ruyi_tt_BIT_OR);
            case '^':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_BIT_XOR_ASS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 1, Ruyi_tt_BIT_XOR);
            case '~':
                choose_tt[0].c = '='; choose_tt[0].token_type = Ruyi_tt_BIT_INVERSE_ASS;
                return ruyi_lexer_handle_mul_tokens(reader, pc, choose_tt, 1, Ruyi_tt_BIT_INVERSE);
            case '?':
                return ruyi_lexer_make_token(Ruyi_tt_QM, pc);
            default:
                break;
        }
    }
    return NULL;
}

BOOL ruyi_lexer_reader_consume_token_if_match(ruyi_lexer_reader *reader, ruyi_token_type type, ruyi_token* out_token) {
    assert(reader);
    ruyi_token* token = ruyi_lexer_reader_next_token(reader);
    if (token == NULL) {
        return FALSE;
    }
    if (out_token) {
        *out_token = *token;
    }
    if (token->type == type) {
        ruyi_lexer_token_destroy(token);
        return TRUE;
    } else {
        ruyi_lexer_reader_push_front(reader, token);
        return FALSE;
    }
}

ruyi_token_type ruyi_lexer_reader_peek_token_type(ruyi_lexer_reader *reader) {
    assert(reader);
    ruyi_token* token = ruyi_lexer_reader_next_token(reader);
    if (token == NULL) {
        return Ruyi_tt_END;
    }
    ruyi_token_type type = token->type;
    ruyi_lexer_reader_push_front(reader, token);
    return type;
}

void ruyi_lexer_reader_push_front(ruyi_lexer_reader *reader, ruyi_token *token) {
    assert(reader);
    assert(token);
    ruyi_list_add_first(reader->token_buffer_queue, ruyi_value_ptr(token));
}

ruyi_token* ruyi_lexer_reader_next_token(ruyi_lexer_reader *reader) {
    assert(reader);
    ruyi_value value;
    ruyi_token* token;
    if (!ruyi_list_empty(reader->token_buffer_queue)) {
        ruyi_list_remove_first(reader->token_buffer_queue, &value);
        token = (ruyi_token*)value.data.ptr;
    } else {
        token = ruyi_lexer_next_token_impl(reader);
    }
    ruyi_lexer_set_snapshot(&reader->token_snapshot, token);
    return token;
}

void ruyi_lexer_reader_consume_token(ruyi_lexer_reader *reader) {
    ruyi_token * token = ruyi_lexer_reader_next_token(reader);
    if (token) {
        ruyi_lexer_set_snapshot(&reader->token_snapshot, token);
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
