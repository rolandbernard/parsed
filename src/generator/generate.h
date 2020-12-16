#ifndef _GENERATE_H_
#define _GENERATE_H_

#include <stdio.h>

#include "parser/ast.h"

void generateParser(FILE* output, Ast* ast);

#endif