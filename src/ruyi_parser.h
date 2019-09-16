//
//  ruyi_parser.h
//  ruyi
//
//  Created by Songli Huang on 2019/8/29.
//  Copyright © 2019 Songli Huang. All rights reserved.
//

#ifndef ruyi_parser_h
#define ruyi_parser_h

#include "ruyi_ast.h"
#include "ruyi_error.h"
#include "ruyi_lexer.h"

ruyi_error* ruyi_parse_ast(ruyi_lexer_reader *reader, ruyi_ast **out_ast);


#endif /* ruyi_parser_h */
