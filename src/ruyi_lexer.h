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
    Ruyi_tt_STRING,         // example: "hello", in unicode
    Ruyi_tt_CHAR,           // example: 'A', in unicode
    Ruyi_tt_LINE_COMMENTS,  // example: // xxxxx
    Ruyi_tt_MLINES_COMMENTS,// example: /* xxx\nxx\n */
    Ruyi_tt_ADD,            // +
    Ruyi_tt_SUB,            // -
    Ruyi_tt_MUL,            // *
    Ruyi_tt_DIV,            // /
    Ruyi_tt_MOD,            // %
    Ruyi_tt_ADD_ASS,        // +=
    Ruyi_tt_SUB_ASS,        // -=
    Ruyi_tt_MUL_ASS,        // *=
    Ruyi_tt_DIV_ASS,        // /=
    Ruyi_tt_MOD_ASS,        // %=
    Ruyi_tt_INC,            // ++
    Ruyi_tt_DEC,            // --
    
    Ruyi_tt_LPAREN,         // (
    Ruyi_tt_RPAREN,         // (
    Ruyi_tt_LBRACKET,       // [
    Ruyi_tt_RBRACKET,       // ]
    Ruyi_tt_LBRACE,         // {
    Ruyi_tt_RBRACE,         // }
    Ruyi_tt_COMMA,          // ,
    Ruyi_tt_SEMICOLON,      // ;
    Ruyi_tt_COLON,          // :
    Ruyi_tt_COLON_ASSIGN,   // :=
    Ruyi_tt_LT,             // <
    Ruyi_tt_GT,             // >
    Ruyi_tt_ASSIGN,         // =
    Ruyi_tt_EQUALS,         // ==
    Ruyi_tt_LTE,            // <=
    Ruyi_tt_GTE,            // >=
    Ruyi_tt_NOT_EQUALS,     // !=
    Ruyi_tt_SHFT_LEFT,      // <<
    Ruyi_tt_SHFT_RIGHT,     // >>
    Ruyi_tt_SHFT_LEFT_ASS,  // <<=
    Ruyi_tt_SHFT_RIGHT_ASS, // >>=
    Ruyi_tt_BIT_AND,        // &
    Ruyi_tt_BIT_OR,         // |
    Ruyi_tt_BIT_XOR,        // ^
    Ruyi_tt_BIT_INVERSE,    // ~
    Ruyi_tt_BIT_AND_ASS,    // &=
    Ruyi_tt_BIT_OR_ASS,     // |=
    Ruyi_tt_BIT_XOR_ASS,    // ^=
    Ruyi_tt_BIT_INVERSE_ASS,// ~=
    Ruyi_tt_LOGIC_AND,      // &&
    Ruyi_tt_LOGIC_OR,       // ||
    Ruyi_tt_LOGIC_NOT,      // !
    Ruyi_tt_QM,             // ?
    Ruyi_tt_DOT,            // .
    Ruyi_tt_DOT3,           // ...
    Ruyi_tt_EOL,            // \n
    Ruyi_tt_END,            // EOF
    
    Ruyi_tt_KW_IF,          // if
    Ruyi_tt_KW_ELSE,        // else
    Ruyi_tt_KW_ELSEIF,      // elseif
    Ruyi_tt_KW_WHILE,       // while
    Ruyi_tt_KW_FOR,         // for
    Ruyi_tt_SWITCH,         // switch
    Ruyi_tt_THIS,           // this
    Ruyi_tt_KW_RETURN,      // return
    Ruyi_tt_KW_BREAK,       // break
    Ruyi_tt_KW_CASE,        // case
    Ruyi_tt_KW_CONTINUE,    // continue
    Ruyi_tt_KW_DEFAULT,     // default
    
    Ruyi_tt_KW_THIS,        // this
    Ruyi_tt_KW_FUNC,        // func
    Ruyi_tt_KW_CLASS,       // class
    Ruyi_tt_KW_NEW,         // new
    Ruyi_tt_KW_VAR,         // var
    Ruyi_tt_KW_INSTANCEOF,  // instanceof

    Ruyi_tt_KW_BOOL,        // bool
    Ruyi_tt_KW_INT,         // int
    Ruyi_tt_KW_FLOAT,       // float
    Ruyi_tt_KW_RUNE,        // rune
    Ruyi_tt_KW_BYTE,        // byte
    Ruyi_tt_KW_DOUBLE,      // double
    Ruyi_tt_KW_LONG,        // long
    Ruyi_tt_KW_SHORT,       // short
    
    Ruyi_tt_KW_ARRAY,       // array
    Ruyi_tt_KW_MAP,         // map
    
    Ruyi_tt_KW_TRY,         // try
    Ruyi_tt_KW_CATCH,       // catch
    Ruyi_tt_KW_THROW,       // throw
    
    Ruyi_tt_KW_TRUE,        // true
    Ruyi_tt_KW_FALSE,       // false
    Ruyi_tt_KW_NULL,        // null

    Ruyi_tt_KW_STATIC,       // static
    Ruyi_tt_KW_ENUM,         // enum
    Ruyi_tt_KW_DO,           // do
    Ruyi_tt_KW_GOTO,         // goto
    Ruyi_tt_KW_CHAR,         // char
    Ruyi_tt_KW_CONST,        // const
    Ruyi_tt_KW_PACKAGE,      // package
    Ruyi_tt_KW_IMPORT,       // import
    
} ruyi_token_type;

typedef struct _ruyi_token {
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

#define LEXER_TOKEN_SNAPSHOT_STR_SIZE 128

typedef struct {
    ruyi_token_type type;
    UINT32 line;
    UINT32 column;
    UINT32 size;
    union {
        INT64 int_value;
        double float_value;
    } value;
    WIDE_CHAR str_snapshot[LEXER_TOKEN_SNAPSHOT_STR_SIZE];
} ruyi_token_snapshot;

typedef struct _ruyi_lexer_reader {
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
    ruyi_token_snapshot token_snapshot;
} ruyi_lexer_reader;

ruyi_lexer_reader* ruyi_lexer_reader_open(ruyi_file *file);

void ruyi_lexer_reader_close(ruyi_lexer_reader *reader);

ruyi_token * ruyi_lexer_reader_next_token(ruyi_lexer_reader *reader);

void ruyi_lexer_reader_consume_token(ruyi_lexer_reader *reader);

void ruyi_lexer_reader_push_front(ruyi_lexer_reader *reader, ruyi_token *token);

ruyi_token_type ruyi_lexer_reader_peek_token_type(ruyi_lexer_reader *reader);

void ruyi_lexer_token_destroy(ruyi_token * token);

BOOL ruyi_lexer_reader_consume_token_if_match(ruyi_lexer_reader *reader, ruyi_token_type type, ruyi_token* out_token);

// keywords
ruyi_token_type ruyi_lexer_keywords_get_type(ruyi_unicode_string * token_value);

// keywords
ruyi_unicode_string* ruyi_lexer_keywords_get_str(ruyi_token_type type);

#endif /* ruyi_lexer_h */
