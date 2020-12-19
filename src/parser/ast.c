
#include <stdlib.h>

#include "ast.h"

void freeAst(Ast* ast) {
    switch (ast->type) {
    case AST_ROOT: {
        AstRoot* root = (AstRoot*)ast;
        for (int i = 0; i < root->child_count; i++) {
            freeAst(root->children[i]);
        }
        free(root->children);
    } break;
    case AST_IDENTIFIER:
        break;
    case AST_DEFINITION: {
        AstDefinition* def = (AstDefinition*)ast;
        freeAst((Ast*)def->ident);
        freeAst((Ast*)def->definition);
    } break;
    case AST_OPTION: {
        AstOption* opt = (AstOption*)ast;
        for (int i = 0; i < opt->option_count; i++) {
            freeAst((Ast*)opt->options[i]);
        }
        free(opt->options);
    } break;
    case AST_TOKEN:
        break;
    case AST_INLINE_C:
        break;
    case AST_SEQUENCE: {
        AstSequence* seq = (AstSequence*)ast;
        for (int i = 0; i < seq->child_count; i++) {
            freeAst(seq->children[i]);
        }
        free(seq->children);
    } break;
    case AST_SETTING: {
        AstSetting* set = (AstSetting*)ast;
        freeAst(set->value);
    } break;
    default:
        break;
    }
    free(ast);
}

AstRoot* createAstRoot() {
    AstRoot* ret = (AstRoot*)malloc(sizeof(AstRoot));
    ret->type = AST_ROOT;
    ret->child_capacity = 0;
    ret->child_count = 0;
    ret->children = NULL;
    return ret;
}

void addToDynamicAstArray(Ast*** data, int* count, int* capacity, Ast* value) {
    if(*count == *capacity) {
        if(*capacity == 0) {
            *capacity = 4;
        } else {
            *capacity *= 2;
        }
        *data = (Ast**)realloc(*data, sizeof(Ast*) * *capacity);
    }
    (*data)[*count] = value;
    (*count)++;
}

void addChildToAstRoot(AstRoot* root, Ast* child) {
    addToDynamicAstArray(&root->children, &root->child_count, &root->child_capacity, child);
}

AstIdentifier* createAstIdentifier(const char* str, int len, int offset) {
    AstIdentifier* ret = (AstIdentifier*)malloc(sizeof(AstIdentifier));
    ret->type = AST_IDENTIFIER;
    ret->ident = str;
    ret->ident_len = len;
    ret->offset = offset;
    return ret;
}

AstDefinition* createAstDefinition(AstIdentifier* ident, AstOption* def, int offset) {
    AstDefinition* ret = (AstDefinition*)malloc(sizeof(AstDefinition));
    ret->type = AST_DEFINITION;
    ret->ident = ident;
    ret->definition = def;
    ret->offset = offset;
    return ret;
}

AstOption* createAstOption() {
    AstOption* ret = (AstOption*)malloc(sizeof(AstOption));
    ret->type = AST_OPTION;
    ret->option_capacity = 0;
    ret->option_count = 0;
    ret->options = NULL;
    return ret;
}

void addOptionToAstOption(AstOption* ast, AstSequence* child) {
    addToDynamicAstArray((Ast***)&ast->options, &ast->option_count, &ast->option_capacity, (Ast*)child);
}

AstToken* createAstToken(bool is_regex, const char* str, int len, int offset) {
    AstToken* ret = (AstToken*)malloc(sizeof(AstToken));
    ret->type = AST_TOKEN;
    ret->is_regex = is_regex;
    ret->pattern = str;
    ret->pattern_len = len;
    ret->offset = offset;
    return ret;
}

AstInlineC* createAstInlineC(const char* str, int len, int offset) {
    AstInlineC* ret = (AstInlineC*)malloc(sizeof(AstInlineC));
    ret->type = AST_INLINE_C;
    ret->src = str;
    ret->len = len;
    ret->offset = offset;
    return ret;
}

AstSequence* createAstSequence() {
    AstSequence* ret = (AstSequence*)malloc(sizeof(AstSequence));
    ret->type = AST_SEQUENCE;
    ret->child_capacity = 0;
    ret->child_count = 0;
    ret->children = NULL;
    return ret;
}

void addChildToAstSequence(AstSequence* seq, Ast* child) {
    addToDynamicAstArray((Ast***)&seq->children, &seq->child_count, &seq->child_capacity, (Ast*)child);
}

AstSetting* createAstSetting(const char* name, int name_len, Ast* value, int offset) {
    AstSetting* ret = (AstSetting*)malloc(sizeof(AstSetting));
    ret->type = AST_SETTING;
    ret->name = name;
    ret->name_len = name_len;
    ret->value = value;
    ret->offset = offset;
    return ret;
}
