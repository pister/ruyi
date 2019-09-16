//
//  ruyi_error.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/23.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_error.h"
#include "ruyi_lexer.h"

#include <string.h>
#include "ruyi_mem.h"

ruyi_token_pos ruyi_token_pos_make(struct _ruyi_token* token) {
    ruyi_token_pos pos = {0};
    if (!token) {
        return pos;
    }
    pos.column = token->column;
    pos.line = token->line;
    pos.width = token->size;
    return pos;
}

ruyi_error * ruyi_error_make(ruyi_error_type type, const char *message, struct _ruyi_token* token) {
    return ruyi_error_make_by_pos(type, message, ruyi_token_pos_make(token));
}

ruyi_error * ruyi_error_make_by_pos(ruyi_error_type type, const char *message, ruyi_token_pos pos) {
    UINT32 len;
    ruyi_error * err = (ruyi_error *)ruyi_mem_alloc(sizeof(ruyi_error));
    err->column = pos.column;
    err->line = pos.line;
    err->type = type;
    err->width = pos.width;
    if (message) {
        len = (UINT32)(strlen(message)) + 1;
        err->message = ruyi_mem_alloc(len);
        memcpy(err->message, message, len);
    }
    return err;
}

void ruyi_error_destroy(ruyi_error * err) {
    if (!err) {
        return;
    }
    if (err->message) {
        ruyi_mem_free(err->message);
    }
    ruyi_mem_free(err);
}
