
#include <assert.h>
#include <string.h>

#include "parser.h"

typedef struct {
    GeneratorSettings* settings;
    FILE* output;
} SingleGenUserData;

static void generateSingleFunction(NonTerminal nonterminal, SingleGenUserData* data) {
    fputs("\nParsedToken* parsed_", data->output);
    fwrite(nonterminal.name, 1, nonterminal.name_len, data->output);
    fputs("(ParsedToken* parsed_tokens, ", data->output);
    if (data->settings->return_type == NULL) {
        fputs(" void* parsed_return", data->output);
    } else {
        fwrite(data->settings->return_type->src, 1, data->settings->return_type->len, data->output);
        fputs("* parsed_return", data->output);
    }
    if (data->settings->args != NULL) {
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

static int compareSequences(AstSequence* s1, AstSequence* s2) {
    for (int i = 0; i < s1->child_count && i < s2->child_count; i++) {
        if(s1->children[i]->type != s2->children[i]->type) {
            return s1->children[i]->type - s2->children[i]->type;
        } else if(s1->children[i]->type == AST_TOKEN) {
            AstToken* tk1 = (AstToken*)s1->children[i];
            AstToken* tk2 = (AstToken*)s2->children[i];
            return tk1->id - tk2->id;
        } else if(s1->children[i]->type == AST_IDENTIFIER) {
            AstIdentifier* iden1 = (AstIdentifier*)s1->children[i];
            AstIdentifier* iden2 = (AstIdentifier*)s2->children[i];
            return iden1->id - iden2->id;
        }
    }
    return s1->child_count - s2->child_count;
}

static void quickSortOptionSequences(AstSequence** sequences, int start, int end) {
    if (start + 2 == end) {
        if(compareSequences(sequences[start], sequences[start + 1]) > 0) {
            AstSequence* tmp = sequences[start];
            sequences[start] = sequences[start + 1];
            sequences[start + 1] = tmp;
        }
    } else if (start + 1 < end) {
        int order_start = start + 1;
        int order_end = end;
        AstSequence* pivot = sequences[start];
        while(order_start < order_end) {
            if(compareSequences(sequences[order_start], pivot) > 0) {
                order_end--;
                AstSequence* tmp = sequences[order_start];
                sequences[order_start] = sequences[order_end];
                sequences[order_end] = tmp;
            } else {
                order_start++;
            }
        }
        AstSequence* tmp = sequences[order_start - 1];
        sequences[order_start - 1] = sequences[start];
        sequences[start] = tmp;
        quickSortOptionSequences(sequences, start, order_start - 1);
        quickSortOptionSequences(sequences, order_end, end);
    }
}

static void generateLeftRecursiveParser(FILE* output, AstOption* option, GeneratorSettings* settings, ErrorContext* error_context) {
    
}

static void generateNonRecursiveParser(FILE* output, AstOption* option, GeneratorSettings* settings, ErrorContext* error_context) {
    quickSortOptionSequences(option->options, 0, option->option_count);
}

void generateParser(FILE* output, Ast* ast, GeneratorSettings* settings, ErrorContext* error_context) {
    assert(ast->type == AST_ROOT);
    AstRoot* root = (AstRoot*)ast;
    for (int i = 0; i < root->child_count; i++) {
        if (root->children[i]->type == AST_DEFINITION) {
            AstDefinition* def = (AstDefinition*)root->children[i];
            fputs("\nParsedToken* parsed_", output);
            fwrite(def->ident->ident, 1, def->ident->ident_len, output);
            fputs("(ParsedToken* parsed_tokens, ", output);
            if (settings->return_type == NULL) {
                fputs(" void* parsed_return", output);
            } else {
                fwrite(settings->return_type->src, 1, settings->return_type->len, output);
                fputs("* parsed_return", output);
            }
            if (settings->args != NULL) {
                fputs(", ", output);
                fwrite(settings->args->src, 1, settings->args->len, output);
            }
            fputs(") {\n", output);
            bool is_left_recursive = false;
            for (int j = 0; !is_left_recursive && j < def->definition->option_count; j++) {
                AstSequence* option = (AstSequence*)def->definition->options[j];
                if (option->child_count > 0 && option->children[0]->type == AST_IDENTIFIER) {
                    AstIdentifier* first = (AstIdentifier*)option->children[0];
                    if (first->id == def->id) {
                        is_left_recursive = true;
                    }
                }
            }
            if (is_left_recursive) {
                generateLeftRecursiveParser(output, def->definition, settings, error_context);
            } else {
                generateNonRecursiveParser(output, def->definition, settings, error_context);
            }
            fputs("}\n", output);
        }
    }
}
