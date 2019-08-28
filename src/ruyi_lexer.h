//
//  ruyi_lexer.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/20.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_lexer_h
#define ruyi_lexer_h

#include "ruyi_basics.h"
#include "ruyi_list.h"
#include "ruyi_io.h"
#include "ruyi_hashtable.h"
#include "ruyi_unicode.h"
#include <stdio.h>

typedef enum {
    Ruyi_tt_IDENTITY,       // example: name1
    Ruyi_tt_INTEGER,        // example: 1234
    Ruyi_tt_FLOAT,          // example: 1234.5678
    Ruyi_tt_STRING,         // example: "hello"
    Ruyi_tt_CHAR,           // example: 'A', in unicode
    Ruyi_tt_LINE_COMMENTS,  // example: // xxxxx
    Ruyi_tt_MLINES_COMMENTS,// example: /* xxx\nxx\n */
    Ruyi_tt_ADD,            // +
    Ruyi_tt_SUB,            // -
    Ruyi_tt_MUL,            // *
    Ruyi_tt_DIV,            // /
    Ruyi_tt_MOD,            // %
    Ruyi_tt_EOL,            // \n
    Ruyi_tt_END,            // EOF
    Ruyi_tt_DOT,     // .
} ruyi_token_type;

typedef struct {
    ruyi_token_type type;
    UINT32 line;
    UINT32 column;
    UINT32 size;
    union {
        INT64 int_value;
        double float_value;
        ruyi_unicode_string* str_value;
    } value;
} ruyi_token;

typedef struct {
    ruyi_list *token_buffer_queue;
    ruyi_unicode_file *file;
    /*
     2-items as a wide_char:
     first: char value as type WIDE_CHAR
     second: char pos as type UINT64(line,column)
     */
    ruyi_list *chars_buffer_queue;
    UINT32 line;
    UINT32 column;
} ruyi_lexer_reader;

ruyi_lexer_reader* ruyi_lexer_reader_open(ruyi_file *file);

void ruyi_lexer_reader_close(ruyi_lexer_reader *reader);

ruyi_token * ruyi_lexer_reader_next_token(ruyi_lexer_reader *reader);

void ruyi_lexer_reader_consume_token(ruyi_lexer_reader *reader);

void ruyi_lexer_reader_push_back(ruyi_lexer_reader *reader, ruyi_token *token);

ruyi_token_type ruyi_lexer_reader_peek_token_type(ruyi_lexer_reader *reader);

void ruyi_lexer_token_destroy(ruyi_token * token);

#endif /* ruyi_lexer_h */
