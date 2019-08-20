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
    reader->buffer_queue = ruyi_list_create();
    return reader;
}

void ruyi_lexer_reader_close(ruyi_lexer_reader *reader) {
    assert(reader);
    ruyi_list_destroy(reader->buffer_queue);
    ruyi_file_close(reader->file);
    ruyi_mem_free(reader);
}

BOOL ruyi_lexer_next_token(ruyi_lexer_reader *reader, ruyi_token *token) {
    // todo
    
    return TRUE;
}
