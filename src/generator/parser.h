#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdio.h>

#include "nonterminal.h"
#include "errors.h"
#include "settings.h"
#include "parser/ast.h"

void generateParserFunctionDeclatations(FILE* output, NonTerminalTable* nonterminals, GeneratorSettings* settings);

void generateParser(FILE* output, Ast* ast, GeneratorSettings* settings, ErrorContext* error_context);

#endif