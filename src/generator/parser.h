#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdio.h>

#include "terminal.h"
#include "errors.h"
#include "settings.h"
#include "parser/ast.h"

void generateParser(FILE* output, Ast* ast, GeneratorSettings* settings, ErrorContext* error_context);

#endif