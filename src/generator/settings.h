#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "errors.h"
#include "parser/ast.h"

typedef struct {
    
} GeneratorSettings;

void fillSettingsFromAst(GeneratorSettings* settings, Ast* ast, ErrorContext* error_context);

#endif