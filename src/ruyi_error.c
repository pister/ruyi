//
//  ruyi_error.c
//  ruyi
//
//  Created by Songli Huang on 2019/8/23.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#include "ruyi_error.h"
#include "ruyi_lexer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ruyi_mem.h"
#include "ruyi_lexer.h"

#define RUYI_ERROR_MESSAGE_BUF_SIZE 2048
#define NAME_BUF_LENGTH 128

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


ruyi_error * ruyi_error_syntax(const char *format, ...) {
    va_list vargs;
    INT32 len;
    char buf[RUYI_ERROR_MESSAGE_BUF_SIZE];
    ruyi_error * err = (ruyi_error *)ruyi_mem_alloc(sizeof(ruyi_error));
    va_start(vargs, format);
    err->column = 0;
    err->line = 0;
    err->type = Ruyi_et_Syntax;
    err->width = 0;
    if (format) {
        len = vsnprintf(buf, RUYI_ERROR_MESSAGE_BUF_SIZE, format, vargs);
        if (len < 0) {
            len = (INT32)strlen(format);
            err->message = ruyi_mem_alloc(len + 1);
            err->message[len] = '\0';
            memcpy(err->message, format, len);
        } else {
            err->message = ruyi_mem_alloc(len + 1);
            err->message[len] = '\0';
            memcpy(err->message, buf, len);
        }
    } else {
        err->message = NULL;
    }
    va_end(vargs);
    return err;
}

ruyi_error * ruyi_error_misc(const char *format, ...) {
    va_list vargs;
    INT32 len;
    char buf[RUYI_ERROR_MESSAGE_BUF_SIZE];
    ruyi_error * err = (ruyi_error *)ruyi_mem_alloc(sizeof(ruyi_error));
    va_start(vargs, format);
    err->column = 0;
    err->line = 0;
    err->type = Ruyi_et_Misc;
    err->width = 0;
    if (format) {
        len = vsnprintf(buf, RUYI_ERROR_MESSAGE_BUF_SIZE, format, vargs);
        if (len < 0) {
            len = (INT32)strlen(format);
            err->message = ruyi_mem_alloc(len + 1);
            err->message[len] = '\0';
            memcpy(err->message, format, len);
        } else {
            err->message = ruyi_mem_alloc(len + 1);
            err->message[len] = '\0';
            memcpy(err->message, buf, len);
        }
    } else {
        err->message = NULL;
    }
    va_end(vargs);
    return err;
}

ruyi_error* ruyi_error_misc_unicode_name(const char* fmt, const ruyi_unicode_string *name) {
    char temp_name[NAME_BUF_LENGTH];
    ruyi_unicode_string_encode_utf8_n(name, temp_name, NAME_BUF_LENGTH-1);
    return ruyi_error_misc(fmt, temp_name);
}

ruyi_error * ruyi_error_by_parser(struct _ruyi_lexer_reader * reader, const char *format, ...) {
    va_list vargs;
    UINT32 len;
    ruyi_token_snapshot * ts = &reader->token_snapshot;
    char buf[RUYI_ERROR_MESSAGE_BUF_SIZE];
    ruyi_error * err = (ruyi_error *)ruyi_mem_alloc(sizeof(ruyi_error));
    va_start(vargs, format);
    err->column = ts->column;
    err->line = ts->line;
    err->type = Ruyi_et_Parser;
    err->width = ts->size;
    if (format) {
        len = vsnprintf(buf, RUYI_ERROR_MESSAGE_BUF_SIZE, format, vargs);
        if (len < 0) {
            len = (INT32)strlen(format);
            err->message = ruyi_mem_alloc(len + 1);
            err->message[len] = '\0';
            memcpy(err->message, format, len);
        } else {
            err->message = ruyi_mem_alloc(len + 1);
            err->message[len] = '\0';
            memcpy(err->message, buf, len);
        }
    } else {
        err->message = NULL;
    }
    /*
    if (message) {
        len = (UINT32)(strlen(message)) + 1;
        err->message = ruyi_mem_alloc(len);
        memcpy(err->message, message, len);
    }
     */
    va_end(vargs);
    return err;
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
