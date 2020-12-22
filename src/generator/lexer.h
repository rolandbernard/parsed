#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdio.h>

#include "terminal.h"
#include "errors.h"
#include "settings.h"

void generateLexerFunctionDeclatations(FILE* output, GeneratorSettings* settings);

void generateLexer(FILE* output, TerminalTable* terminals, GeneratorSettings* settings, ErrorContext* error_context);

#endif