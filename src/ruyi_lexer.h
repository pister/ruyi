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
#include <stdio.h>

typedef enum {
    ruyi_tt_IDENTITY,
    ruyi_tt_INTEGER,
    ruyi_tt_STRING,
    ruyi_tt_EOL,
    ruyi_tt_END,
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
        WIDE_CHAR *unicode;
        BYTE *bytes;
    } value;
} ruyi_token;

typedef struct {
    ruyi_list *buffer_queue;
    ruyi_file *file;
} ruyi_lexer_reader;

ruyi_lexer_reader* ruyi_lexer_reader_open(ruyi_file *file);
void ruyi_lexer_reader_close(ruyi_lexer_reader *reader);

BOOL ruyi_lexer_next_token(ruyi_lexer_reader *reader, ruyi_token *token);


#endif /* ruyi_lexer_h */
