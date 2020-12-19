
#include <assert.h>

#include "parser.h"

typedef struct {
    GeneratorSettings* settings;
    FILE* output;
} SingleGenUserData;

static void generateSingleFunction(NonTerminal nonterminal, SingleGenUserData* data) {
    fputs("\n", data->output);
    fputs("ParsedToken* parsed_", data->output);
    fwrite(nonterminal.name, 1, nonterminal.name_len, data->output);
    fputs("(ParsedToken* parsed_tokens, ", data->output);
    if(data->settings->return_type == NULL) {
        fputs(" void* parsed_return", data->output);
    } else {
        fwrite(data->settings->return_type->src, 1, data->settings->return_type->len, data->output);
        fputs("* parsed_return", data->output);
    }
    if(data->settings->args != NULL) {
        fputs(", " , data->output);
        fwrite(data->settings->args->src, 1, data->settings->args->len, data->output);
    }
    fputs(");\n", data->output);
}

void generateParserFunctionDeclatations(FILE* output, NonTerminalTable* nonterminals, GeneratorSettings* settings) {
    SingleGenUserData data = {
        .output = output,
        .settings = settings,
    };
    forEachInNonTerminalTable(nonterminals, (NonTerminalTableIterationFunction)generateSingleFunction, (void*)&data);
}

void generateParser(FILE* output, Ast* ast, GeneratorSettings* settings, ErrorContext* error_context) {
    assert(ast->type == AST_ROOT);
}
