
#include <assert.h>

#include "nonterminal.h"
#include "terminal.h"
#include "lexer.h"
#include "parser.h"

#include "generate.h"

static void searchForTokens(Ast* ast, TerminalTable* terminals, NonTerminalTable* nonterminals, ErrorContext* error_context) {
    switch (ast->type) {
    case AST_ROOT: {
        AstRoot* root = (AstRoot*)ast;
        for (int i = 0; i < root->child_count; i++) {
            searchForTokens(root->children[i], terminals, nonterminals, error_context);
        }
    } break;
    case AST_IDENTIFIER: {
        AstIdentifier* ident = (AstIdentifier*)ast;
        NonTerminal token = {
            .id = -1,
            .name = ident->ident,
            .name_len = ident->ident_len,
            .defined = false,
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
        token = getFromNonTerminalTable(nonterminals, def->ident->ident, def->ident->ident_len);
        if (token.defined) {
            addError(error_context, "Duplicate non terminal name", def->offset, ERROR);
        } else if (token.id != -1) {
            def->id = token.id;
        } else {
            token.id = -1;
            token.name = def->ident->ident;
            token.name_len = def->ident->ident_len;
            token.defined = true,
            def->id = addToNonTerminalTable(nonterminals, token);
        }
        def->ident->id = def->id;
        searchForTokens((Ast*)def->definition, terminals, nonterminals, error_context);
    } break;
    case AST_OPTION: {
        AstOption* opt = (AstOption*)ast;
        for (int i = 0; i < opt->option_count; i++) {
            searchForTokens((Ast*)opt->options[i], terminals, nonterminals, error_context);
        }
    } break;
    case AST_TOKEN: {
        AstToken* tkn = (AstToken*)ast;
        Terminal token = {
            .id = -1,
            .is_regex = tkn->is_regex,
            .is_code = false,
            .pattern = tkn->pattern,
            .pattern_len = tkn->pattern_len,
            .offset = tkn->offset,
        };
        tkn->id = addToTerminalTable(terminals, token);
    } break;
    case AST_INLINE_C: {
        AstInlineC* code = (AstInlineC*)ast;
        if (code->is_token) {
            Terminal token = {
                .id = -1,
                .is_regex = false,
                .is_code = true,
                .pattern = code->src,
                .pattern_len = code->len,
                .offset = code->offset,
            };
            code->id = addToTerminalTable(terminals, token);
        }
    } break;
    case AST_SEQUENCE: {
        AstSequence* seq = (AstSequence*)ast;
        for (int i = 0; i < seq->child_count; i++) {
            searchForTokens(seq->children[i], terminals, nonterminals, error_context);
        }
    } break;
    case AST_SETTING: {
        AstSetting* set = (AstSetting*)ast;
        searchForTokens(set->value, terminals, nonterminals, error_context);
    } break;
    default:
        break;
    }
}

static void writePreDefInlineC(FILE* output, Ast* ast) {
    assert(ast->type == AST_ROOT);
    AstRoot* root = (AstRoot*)ast;
    for (int i = 0; i < root->child_count; i++) {
        if (root->children[i]->type == AST_INLINE_C) {
            AstInlineC* code = (AstInlineC*)root->children[i];
            fwrite(code->src, 1, code->len, output);
        } else {
            break;
        }
    }
}

static void writePostDefInlineC(FILE* output, Ast* ast) {
    assert(ast->type == AST_ROOT);
    AstRoot* root = (AstRoot*)ast;
    int i = 0;
    for(; i < root->child_count; i++) {
        if(root->children[i]->type != AST_INLINE_C) {
            break;
        }
    }
    for(; i < root->child_count; i++) {
        if(root->children[i]->type == AST_INLINE_C) {
            AstInlineC* code = (AstInlineC*)root->children[i];
            fwrite(code->src, 1, code->len, output);
        }
    }
}

void generateLexerAndParser(FILE* output, Ast* ast, ErrorContext* error_context) {
    NonTerminalTable nonterminals;
    initNonTerminalTable(&nonterminals);
    TerminalTable terminals;
    initTerminalTable(&terminals);
    searchForTokens(ast, &terminals, &nonterminals, error_context);

    GeneratorSettings settings;
    initSettings(&settings);
    fillSettingsFromAst(&settings, ast, error_context);
    
    writePreDefInlineC(output, ast);
    generateLexerFunctionDeclatations(output, &settings);
    generateParserFunctionDeclatations(output, &nonterminals, &settings);
    writePostDefInlineC(output, ast);
    generateLexer(output, &terminals, &settings, error_context);
    generateParser(output, ast, &settings, error_context);

    freeSettings(&settings);
    freeTerminalTable(&terminals);
    freeNonTerminalTable(&nonterminals);
}