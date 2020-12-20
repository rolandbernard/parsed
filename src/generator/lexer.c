
#include <ctype.h>
#include <string.h>

#include "regex.h"

#include "lexer.h"

static void generateTokenTypeDefinitions(FILE* output) {
    fputs("\ntypedef struct {\n", output);
    fputs("    int kind;\n", output);
    fputs("    int offset;\n", output);
    fputs("    int len;\n", output);
    fputs("    const char* start;\n", output);
    fputs("} ParsedToken;\n", output);
    fputs("\ntypedef ParsedToken* ParsedTokens;\n", output);
}

static void generateTokenDeterminer(FILE* output, TerminalTable* terminals, GeneratorSettings* settings, ErrorContext* error_context) {
    bool to_ignore[terminals->count];
    for(int i = 0; i < terminals->count; i++) {
        to_ignore[i] = false;
    }
    fputs("\nint determineToken(const char* parsed_start, int parsed_max_len, ParsedToken* parsed_out) {\n", output);
    fputs("\tint parsed_length = 0;\n", output);
    for(int i = 0; i < settings->ignore_count; i++) {
        if(settings->ignore_list[i]->type == AST_TOKEN) {
            AstToken* tkn = (AstToken*)settings->ignore_list[i];
            to_ignore[tkn->id] = true;
        } else {
            AstInlineC* code = (AstInlineC*)settings->ignore_list[i];
            fputs("\t{\n", output);
            int last_written = 0;
            for(int j = 0; j < code->len; j++) {
                if(code->src[j] == '$') {
                    fwrite(code->src + last_written, 1, j - last_written, output);
                    int len = 1;
                    while(isalnum(code->src[j + len]) || code->src[j + len] == '_') {
                        len++;
                    }
                    if(strncmp("$length", code->src + j, len) == 0) {
                        fputs(" parsed_length ", output);
                    } else if(strncmp("$source", code->src + j, len) == 0) {
                        fputs(" parsed_start ", output);
                    } else if(strncmp("$maxlength", code->src + j, len) == 0) {
                        fputs(" parsed_max_len ", output);
                    } else {
                        addError(error_context, "Unknown parser variable", code->offset + 1 + j, ERROR);
                        fwrite(code->src + j, 1, len, output);
                    }
                    j += len - 1;
                    last_written = j + 1;
                }
            }
            fwrite(code->src + last_written, 1, code->len - last_written, output);
            fputs("\n\t}\n", output);
        }
    }
    
    fputs("}\n", output);
}

static void addToSortedTerminalArray(Terminal nonterminal, Terminal* array) {
    array[nonterminal.id] = nonterminal;
}

void generateLexer(FILE* output, TerminalTable* terminals, GeneratorSettings* settings, ErrorContext* error_context) {
    Terminal sorted[terminals->count];
    forEachInTerminalTable(terminals, (TerminalTableIterationFunction)addToSortedTerminalArray, (void*)sorted);
    bool is_regexs[terminals->count];
    const char* patterns[terminals->count];
    int lengths[terminals->count];
    for(int i = 0; i < terminals->count; i++) {
        is_regexs[i] = sorted[i].is_regex;
        patterns[i] = sorted[i].pattern;
        lengths[i] = sorted[i].pattern_len;
    }
    Regex dfa = compileMultiMatchingStringsAndRegexN(terminals->count, is_regexs, patterns, lengths);
    if(dfa != NULL) {
        generateTokenTypeDefinitions(output);
        generateTokenDeterminer(output, terminals, settings, error_context);
        disposeRegex(dfa);
    } else {
        for(int i = 0; i < terminals->count; i++) {
            int error = getRegexErrorLocationN(patterns[i], lengths[i]);
            if(is_regexs[i] && error != -1) {
                fprintf(stderr, "%i\n", lengths[i]);
                addError(error_context, "Illegal regex pattern", sorted[i].offset + 1 + error, ERROR);
            }
        }
    }
}
