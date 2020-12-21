
#include <ctype.h>
#include <string.h>

#include "../regex/src/regex.h"

#include "lexer.h"

static void generateTokenTypeDefinitions(FILE* output) {
    fputs("\ntypedef struct {\n", output);
    fputs("\tint kind;\n", output);
    fputs("\tint offset;\n", output);
    fputs("\tint len;\n", output);
    fputs("\tconst char* start;\n", output);
    fputs("} ParsedToken;\n", output);
    fputs("\ntypedef ParsedToken* ParsedTokens;\n", output);
}

static void generateTokenDeterminer(FILE* output, TerminalTable* terminals, Regex dfa, GeneratorSettings* settings, ErrorContext* error_context) {
    bool to_ignore[terminals->count];
    for(int i = 0; i < terminals->count; i++) {
        to_ignore[i] = false;
    }
    fputs("\nint determineToken(const char* parsed_start, int parsed_offset, int parsed_max_len, ParsedToken* parsed_out", output);
    if(settings->args != NULL) {
        fputs(", " , output);
        fwrite(settings->args->src, 1, settings->args->len, output);
    }
    fputs(") {\n", output);
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
            fputs("\tif (parsed_length > 0) {\n", output);
            fprintf(output, "\t\tparsed_out->kind = %i;\n", terminals->count + i);
            fputs("\t\tparsed_out->offset = parsed_offset;\n", output);
            fputs("\t\tparsed_out->len = parsed_length;\n", output);
            fputs("\t\tparsed_out->start = parsed_start;\n", output);
            fputs("\t\treturn 0;\n", output);
            fputs("\t}\n", output);
        }
    }
    fputs("\tint parsed_state = 0;\n", output);
    fputs("\tint parsed_kind = 0;\n", output);
    fputs("\tfor (int parsed_i = 0; parsed_i < parsed_max_len; parsed_i++) {\n", output);
    fputs("\t\tswitch (parsed_state) {\n", output);
    for(int i = 0; i < dfa->num_states; i++) {
        fprintf(output, "\t\tcase %i:\n", i);
        fputs("\t\t\tswitch (parsed_start[parsed_i]) {\n", output);
        for(int c = 0; c < REGEX_NUM_CHARS; c++) {
            if(dfa->states[i][c].state_type == REGEX_STATE_NEXT) {
                if(isprint(c) && c != '\'') {
                    fprintf(output, "\t\t\tcase '%c':\n", (char)c);
                } else {
                    fprintf(output, "\t\t\tcase '\\x%.2x':\n", c);
                }
                if(
                    c + 1 < REGEX_STATE_NEXT || dfa->states[i][c].state_type != dfa->states[i][c + 1].state_type
                    || dfa->states[i][c].next_state != dfa->states[i][c + 1].next_state
                ) {
                    fputs("\t\t\t\tbreak;\n", output);
                }
            }
        }
        fputs("\t\t\tdefault:\n", output);
        fputs("\t\t\t\tbreak;\n", output);
        fputs("\t\t\t}\n", output);
        fputs("\t\t\tbreak;\n", output);
    }
    fputs("\t\t}\n", output);
    fputs("\t}\n", output);
    fputs("break_state_machine:\n", output);
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
        generateTokenDeterminer(output, terminals, dfa, settings, error_context);
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
