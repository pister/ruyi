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
#include <stdio.h>

typedef enum {
    Ruyi_tt_IDENTITY,
    Ruyi_tt_INTEGER,
    Ruyi_tt_STRING,
    Ruyi_tt_EOL,
    Ruyi_tt_END,
} ruyi_token_type;

typedef struct {
    ruyi_token_type type;
    UINT32 line;
    UINT32 column_pos;
    UINT32 token_width;
    UINT32 pointer_length;
    union {
        INT64 int_value;
        double float_value;
    } value;
} ruyi_token;

typedef struct {
    ruyi_list *token_buffer_queue;
    ruyi_file *file;
    ruyi_list *chars_buffer_queue;
    ruyi_hashtable *unicode_pool;
} ruyi_lexer_reader;

ruyi_lexer_reader* ruyi_lexer_reader_open(ruyi_file *file);

void ruyi_lexer_reader_close(ruyi_lexer_reader *reader);

BOOL ruyi_lexer_next_token(ruyi_lexer_reader *reader, ruyi_token *token);

void ruyi_lexer_push_back(ruyi_lexer_reader *reader, const ruyi_token *token);

void ruyi_lexer_peek(ruyi_lexer_reader *reader, ruyi_token *token);

UINT32 ruyi_lexer_get_token_text(const ruyi_lexer_reader *reader, const ruyi_token *token, WIDE_CHAR* out_buf, UINT32 buf_length);


#endif /* ruyi_lexer_h */
