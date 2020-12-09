#ifndef _PARSER_H_
#define _PARSER_H_

#include "ast.h"
#include "errors.h"

Ast* parseGrammar(const char* src, ErrorContext* error_context);

#endif