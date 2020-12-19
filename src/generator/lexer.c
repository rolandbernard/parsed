
#include "regex.h"

#include "lexer.h"

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
        printRegexDfa(dfa);
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
