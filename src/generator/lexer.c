
#include <ctype.h>
#include <string.h>

#include "../regex/src/regex.h"

#include "lexer.h"

void generateLexerFunctionDeclatations(FILE* output, GeneratorSettings* settings) {
    fputs("\n#include <stdlib.h>\n", output);
    fputs("\ntypedef struct {\n", output);
    fputs("\tint kind;\n", output);
    fputs("\tint offset;\n", output);
    fputs("\tint len;\n", output);
    fputs("\tconst char* start;\n", output);
    fputs("} ParsedToken;\n", output);
    fputs("\ntypedef ParsedToken* ParsedTokens;\n", output);
    fputs("\nint parsedDetermineToken(const char* parsed_start, int parsed_offset, int parsed_max_len, ParsedToken* parsed_out", output);
    if (settings->args != NULL) {
        fputs(", ", output);
        fwrite(settings->args->src, 1, settings->args->len, output);
    }
    fputs(");\n", output);
    fputs("\nParsedToken* parsedTokenize(const char* parsed_string, int parsed_length", output);
    if (settings->args != NULL) {
        fputs(", ", output);
        fwrite(settings->args->src, 1, settings->args->len, output);
    }
    fputs(");\n", output);
    fputs("\nchar* parsedTokenToString(ParsedToken token);\n", output);
    fputs("\nint parsedReachedEnd(ParsedToken* tokens);\n", output);
}

static void generateTokenDeterminer(FILE* output, TerminalTable* terminals, Regex dfa, GeneratorSettings* settings, ErrorContext* error_context) {
    bool to_ignore[terminals->count];
    for (int i = 0; i < terminals->count; i++) {
        to_ignore[i] = false;
    }
    fputs("\nint parsedDetermineToken(const char* parsed_start, int parsed_offset, int parsed_max_len, ParsedToken* parsed_out", output);
    if (settings->args != NULL) {
        fputs(", ", output);
        fwrite(settings->args->src, 1, settings->args->len, output);
    }
    fputs(") {\n", output);
    fputs("\tif (parsed_max_len == 0) {\n", output);
    fputs("\t\tparsed_out->kind = -1;\n", output);
    fputs("\t\tparsed_out->offset = parsed_offset;\n", output);
    fputs("\t\tparsed_out->len = 0;\n", output);
    fputs("\t\tparsed_out->start = parsed_start;\n", output);
    fputs("\t\treturn 0;\n", output);
    fputs("\t}\n", output);
    fputs("\tint parsed_length = 0;\n", output);
    for (int i = 0; i < settings->ignore_count; i++) {
        if (settings->ignore_list[i]->type == AST_TOKEN) {
            AstToken* tkn = (AstToken*)settings->ignore_list[i];
            to_ignore[tkn->id] = true;
        } else {
            AstInlineC* code = (AstInlineC*)settings->ignore_list[i];
            fputs("\t{\n", output);
            int last_written = 0;
            for (int j = 0; j < code->len; j++) {
                if (code->src[j] == '$') {
                    fwrite(code->src + last_written, 1, j - last_written, output);
                    int len = 1;
                    while (isalnum(code->src[j + len]) || code->src[j + len] == '_') {
                        len++;
                    }
                    if (strncmp("$length", code->src + j, len) == 0) {
                        fputs(" parsed_length ", output);
                    } else if (strncmp("$source", code->src + j, len) == 0) {
                        fputs(" parsed_start ", output);
                    } else if (strncmp("$maxlength", code->src + j, len) == 0) {
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
            fputs("\t\tparsed_out->kind = -2;\n", output);
            fputs("\t\tparsed_out->offset = parsed_offset;\n", output);
            fputs("\t\tparsed_out->len = parsed_length;\n", output);
            fputs("\t\tparsed_out->start = parsed_start;\n", output);
            fputs("\t\treturn 1;\n", output);
            fputs("\t}\n", output);
        }
    }
    fputs("\tint parsed_state = 0;\n", output);
    fputs("\tint parsed_kind = -2;\n", output);
    fputs("\tint parsed_len = 0;\n", output);
    fputs("\tfor (parsed_len = 0; parsed_len <= parsed_max_len; parsed_len++) {\n", output);
    fputs("\t\tswitch (parsed_state) {\n", output);
    for (int i = 0; i < dfa->num_states; i++) {
        fprintf(output, "\t\tcase %i:\n", i);
        if (dfa->states[i][REGEX_NUM_CHARS].state_type == REGEX_STATE_END) {
            fputs("\t\t\tparsed_length = parsed_len;\n", output);
            if (!to_ignore[dfa->states[i][REGEX_NUM_CHARS].end_point]) {
                fprintf(output, "\t\t\tparsed_kind = %i;\n", dfa->states[i][REGEX_NUM_CHARS].end_point);
            }
        }
        fputs("\t\t\tswitch (parsed_len < parsed_max_len ? parsed_start[parsed_len] : -1) {\n", output);
        for (int c = 0; c < REGEX_NUM_CHARS; c++) {
            if (dfa->states[i][c].state_type == REGEX_STATE_NEXT) {
                if (isprint(c) && c != '\'') {
                    fprintf(output, "\t\t\tcase '%c':\n", (char)c);
                } else {
                    fprintf(output, "\t\t\tcase '\\x%.2x':\n", c);
                }
                if (c + 1 >= REGEX_NUM_CHARS || dfa->states[i][c].state_type != dfa->states[i][c + 1].state_type || dfa->states[i][c].next_state != dfa->states[i][c + 1].next_state) {
                    fprintf(output, "\t\t\t\tparsed_state = %i;\n", dfa->states[i][c].next_state);
                    fputs("\t\t\t\tbreak;\n", output);
                }
            }
        }
        fputs("\t\t\tdefault:\n", output);
        fputs("\t\t\t\tgoto break_state_machine;\n", output);
        fputs("\t\t\t\tbreak;\n", output);
        fputs("\t\t\t}\n", output);
        fputs("\t\t\tbreak;\n", output);
    }
    fputs("\t\t}\n", output);
    fputs("\t}\n", output);
    fputs("break_state_machine:\n", output);
    fputs("\tif (parsed_length > 0) {\n", output);
    fputs("\t\tparsed_out->kind = parsed_kind;\n", output);
    fputs("\t\tparsed_out->offset = parsed_offset;\n", output);
    fputs("\t\tparsed_out->len = parsed_length;\n", output);
    fputs("\t\tparsed_out->start = parsed_start;\n", output);
    fputs("\t\treturn parsed_kind >= 0 ? 0 : 1;\n", output);
    fputs("\t} else {\n", output);
    fputs("\t\tparsed_out->kind = -3;\n", output);
    fputs("\t\tparsed_out->offset = parsed_offset;\n", output);
    fputs("\t\tparsed_out->len = 1;\n", output);
    fputs("\t\tparsed_out->start = parsed_start;\n", output);
    fputs("\t\treturn -1;\n", output);
    fputs("\t}\n", output);
    fputs("}\n", output);
}

static void generateTokenizer(FILE* output, GeneratorSettings* settings) {
    fputs("\nParsedToken* parsedTokenize(const char* parsed_string, int parsed_length", output);
    if (settings->args != NULL) {
        fputs(", ", output);
        fwrite(settings->args->src, 1, settings->args->len, output);
    }
    fputs(") {\n", output);
    fputs("\tint parsed_offset = 0;\n", output);
    fputs("\tint parsed_count = 0;\n", output);
    fputs("\tint parsed_capacity = 16;\n", output);
    fputs("\tParsedToken* parsed_tokens = (ParsedToken*)malloc(sizeof(ParsedToken) * parsed_capacity);\n", output);
    fputs("\tfor (;;) {\n", output);
    fputs("\t\tParsedToken parsed_token;\n", output);
    fputs("\t\tint res = parsedDetermineToken(parsed_string, parsed_offset, parsed_length, &parsed_token", output);
    if (settings->args != NULL) {
        fputs(", ", output);
        printArgNames(output, settings->args);
    }
    fputs(");\n", output);
    fputs("\t\tif(res <= 0) {\n", output);
    fputs("\t\t\tif (parsed_count == parsed_capacity) {\n", output);
    fputs("\t\t\t\tparsed_capacity *= 2;\n", output);
    fputs("\t\t\t\tparsed_tokens = (ParsedToken*)realloc(parsed_tokens, sizeof(ParsedToken) * parsed_capacity);\n", output);
    fputs("\t\t\t}\n", output);
    fputs("\t\t\tparsed_tokens[parsed_count] = parsed_token;\n", output);
    fputs("\t\t\tparsed_count++;\n", output);
    fputs("\t\t\tif (parsed_token.kind == -1) {\n", output);
    fputs("\t\t\t\tbreak;\n", output);
    fputs("\t\t\t}\n", output);
    fputs("\t\t}\n", output);
    fputs("\t\tparsed_offset += parsed_token.len;\n", output);
    fputs("\t\tparsed_string += parsed_token.len;\n", output);
    fputs("\t\tparsed_length -= parsed_token.len;\n", output);
    fputs("\t}\n", output);
    fputs("\tparsed_tokens = (ParsedToken*)realloc(parsed_tokens, sizeof(ParsedToken) * parsed_count);\n", output);
    fputs("\treturn parsed_tokens;\n", output);
    fputs("}\n", output);
}

static void generateUtilFunctions(FILE* output, GeneratorSettings* settings) {
    fputs("\nchar* parsedTokenToString(ParsedToken token) {\n", output);
    fputs("\tchar* ret = malloc(sizeof(char) * token.len);\n", output);
    fputs("\tmemcpy(ret, token.start, sizeof(char) * token.len);\n", output);
    fputs("\treturn ret;\n", output);
    fputs("}\n", output);
    fputs("\nint parsedReachedEnd(ParsedToken* tokens) {\n", output);
    fputs("\treturn tokens->kind == -1;\n", output);
    fputs("}\n", output);
}

static void addToSortedTerminalArray(Terminal nonterminal, Terminal* array) { array[nonterminal.id] = nonterminal; }

void generateLexer(FILE* output, TerminalTable* terminals, GeneratorSettings* settings, ErrorContext* error_context) {
    Terminal sorted[terminals->count];
    forEachInTerminalTable(terminals, (TerminalTableIterationFunction)addToSortedTerminalArray, (void*)sorted);
    bool is_regexs[terminals->count];
    const char* patterns[terminals->count];
    int lengths[terminals->count];
    for (int i = 0; i < terminals->count; i++) {
        is_regexs[i] = sorted[i].is_regex;
        patterns[i] = sorted[i].pattern;
        lengths[i] = sorted[i].pattern_len;
    }
    Regex dfa = compileMultiMatchingStringsAndRegexN(terminals->count, is_regexs, patterns, lengths);
    if (dfa != NULL) {
        generateTokenDeterminer(output, terminals, dfa, settings, error_context);
        generateTokenizer(output, settings);
        generateUtilFunctions(output, settings);
        disposeRegex(dfa);
    } else {
        for (int i = 0; i < terminals->count; i++) {
            int error = getRegexErrorLocationN(patterns[i], lengths[i]);
            if (is_regexs[i] && error != -1) {
                fprintf(stderr, "%i\n", lengths[i]);
                addError(error_context, "Illegal regex pattern", sorted[i].offset + 1 + error, ERROR);
            }
        }
    }
}
