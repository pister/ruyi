//
//  ruyi_error.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/23.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_error_h
#define ruyi_error_h

#include "ruyi_basics.h"
#include "ruyi_unicode.h"

typedef enum {
    Ruyi_et_Lexer, Ruyi_et_Parser, Ruyi_et_Syntax
} ruyi_error_type;

struct _ruyi_token;

typedef struct {
    UINT32 line;
    UINT32 column;
    UINT32 width;
    ruyi_error_type type;
    char *message;
} ruyi_error;

typedef struct {
    UINT32 line;
    UINT32 column;
    UINT32 width;
} ruyi_token_pos;

ruyi_token_pos ruyi_token_pos_make(struct _ruyi_token* token);

ruyi_error * ruyi_error_make(ruyi_error_type type, const char *message, struct _ruyi_token* token);

ruyi_error * ruyi_error_make_by_pos(ruyi_error_type type, const char *message, ruyi_token_pos pos);


void ruyi_error_destroy(ruyi_error * err);



#endif /* ruyi_error_h */
