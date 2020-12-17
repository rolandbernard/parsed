
#include "nonterminal.h"
#include "terminal.h"
#include "regex.h"

#include "generate.h"

void searchForTokens(Ast* ast, TerminalTable* terminals, NonTerminalTable* nonterminals) {
    switch (ast->type) {
    case AST_ROOT: {
        AstRoot* root = (AstRoot*)ast;
        for (int i = 0; i < root->child_count; i++) {
            searchForTokens(root->children[i], terminals, nonterminals);
        }
    } break;
    case AST_IDENTIFIER: {
        AstIdentifier* ident = (AstIdentifier*)ast;
        NonTerminal token = {
            .id = -1,
            .name = ident->ident,
            .name_len = ident->ident_len,
        };
        ident->id = addToNonTerminalTable(nonterminals, token);
    } break;
    case AST_DEFINITION: {
        AstDefinition* def = (AstDefinition*)ast;
        NonTerminal token = {
            .id = -1,
            .name = def->ident->ident,
            .name_len = def->ident->ident_len,
        };
        def->id = addToNonTerminalTable(nonterminals, token);
        searchForTokens((Ast*)def->definition, terminals, nonterminals);
    } break;
    case AST_OPTION: {
        AstOption* opt = (AstOption*)ast;
        for (int i = 0; i < opt->option_count; i++) {
            searchForTokens((Ast*)opt->options[i], terminals, nonterminals);
        }
    } break;
    case AST_TOKEN: {
        AstToken* tkn = (AstToken*)ast;
        Terminal token = {
            .id = -1,
            .is_regex = tkn->is_regex,
            .pattern = tkn->pattern,
            .pattern_len = tkn->pattern_len,
        };
        tkn->id = addToTerminalTable(terminals, token);
    } break;
    case AST_INLINE_C:
        break;
    case AST_SEQUENCE: {
        AstSequence* seq = (AstSequence*)ast;
        for (int i = 0; i < seq->child_count; i++) {
            searchForTokens(seq->children[i], terminals, nonterminals);
        }
    } break;
    case AST_SETTING: {
        AstSetting* set = (AstSetting*)ast;
        searchForTokens(set->value, terminals, nonterminals);
    } break;
    default:
        break;
    }
}

void addToSortedTerminalArray(Terminal nonterminal, Terminal* array) {
    array[nonterminal.id] = nonterminal;
}

void generateScanner(FILE* output, TerminalTable* terminals) {
    Terminal sorted[terminals->count];
    forEachInTerminalTable(terminals, (TerminalTableIterationFunction)addToSortedTerminalArray, (void*)sorted);
    bool is_regexs[terminals->count];
    const char* patterns[terminals->count];
    int lengths[terminals->count];
    for(int i = 0; i < terminals->count; i++) {
        is_regexs[i] = sorted[i].is_regex
        patterns[i] = sorted[i].pattern;
        lengths[i] = sorted[i].pattern_len;
    }
    Regex dfa = compileMultiMatchingStringsAndRegexN(terminals->count, is_regexs, patterns, lengths);
}

void generateParser(FILE* output, Ast* ast) {
    NonTerminalTable nonterminals;
    initNonTerminalTable(&nonterminals);
    TerminalTable terminals;
    initTerminalTable(&terminals);
    searchForTokens(ast, &terminals, &nonterminals);
    
}
