#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "errors.h"
#include "parser/ast.h"

typedef struct {
    int ignore_count;
    int ignore_capacity;
    Ast** ignore_list;
    AstInlineC* return_type;
    AstInlineC* args;
    AstInlineC* free;
} GeneratorSettings;

void initSettings(GeneratorSettings* settings);

void freeSettings(GeneratorSettings* settings);

void fillSettingsFromAst(GeneratorSettings* settings, Ast* ast, ErrorContext* error_context);

#endif