
#include <assert.h>

#include "nonterminal.h"
#include "terminal.h"
#include "lexer.h"
#include "parser.h"

#include "generate.h"

static void searchForTokens(Ast* ast, TerminalTable* terminals, NonTerminalTable* nonterminals) {
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
            .offset = tkn->offset,
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

static void writeInitialInlineC(FILE* output, Ast* ast) {
    assert(ast->type == AST_ROOT);
    AstRoot* root = (AstRoot*)ast;
    for(int i = 0; i < root->child_count; i++) {
        if(root->children[i]->type == AST_INLINE_C) {
            AstInlineC* code = (AstInlineC*)root->children[i];
            fwrite(code->src, 1, code->len, output);
        } else if(root->children[i]->type == AST_DEFINITION) {
            break;
        }
    }
}

void generateLexerAndParser(FILE* output, Ast* ast, ErrorContext* error_context) {
    NonTerminalTable nonterminals;
    initNonTerminalTable(&nonterminals);

    TerminalTable terminals;
    initTerminalTable(&terminals);
    searchForTokens(ast, &terminals, &nonterminals);

    GeneratorSettings settings;
    initSettings(&settings);
    fillSettingsFromAst(&settings, ast, error_context);
    
    writeInitialInlineC(output, ast);
    generateLexer(output, &terminals, &settings, error_context);
    generateParserFunctionDeclatations(output, &nonterminals, &settings);
    generateParser(output, ast, &settings, error_context);

    freeSettings(&settings);
    freeTerminalTable(&terminals);
    freeNonTerminalTable(&nonterminals);
}