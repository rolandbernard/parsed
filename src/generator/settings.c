
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "settings.h"

void initSettings(GeneratorSettings* settings) {
    settings->ignore_count = 0;
    settings->ignore_capacity = 0;
    settings->ignore_list = NULL;
    settings->return_type = NULL;
    settings->args = NULL;
    settings->free = NULL;
}

void freeSettings(GeneratorSettings* settings) {
    free(settings->ignore_list);
}

void fillSettingsFromAst(GeneratorSettings* settings, Ast* ast, ErrorContext* error_context) {
    assert(ast->type == AST_ROOT);
    AstRoot* root = (AstRoot*)ast;
    for(int i = 0; i < root->child_count; i++) {
        if(root->children[i]->type == AST_SETTING) {
            AstSetting* setting = (AstSetting*)root->children[i];
            if(strncmp("args", setting->name, setting->name_len) == 0) {
                if(settings->args != NULL) {
                    addError(error_context, "Found ultiple definitions of %args", setting->offset, ERROR);
                } else if(setting->value->type != AST_INLINE_C) {
                    addError(error_context, "The %args settings must be of inline code type", setting->offset, ERROR);
                } else {
                    settings->args = (AstInlineC*)setting->value;
                }
            } else if(strncmp("return", setting->name, setting->name_len) == 0) {
                if(settings->return_type != NULL) {
                    addError(error_context, "Found multiple definitions of %return", setting->offset, ERROR);
                } else if(setting->value->type != AST_INLINE_C) {
                    addError(error_context, "The %return settings must be of inline code type", setting->offset, ERROR);
                } else {
                    settings->return_type = (AstInlineC*)setting->value;
                }
            } else if(strncmp("free", setting->name, setting->name_len) == 0) {
                if(settings->free != NULL) {
                    addError(error_context, "Found multiple definitions of %free", setting->offset, ERROR);
                } else if(setting->value->type != AST_INLINE_C) {
                    addError(error_context, "The %free settings must be of inline code type", setting->offset, ERROR);
                } else {
                    settings->free = (AstInlineC*)setting->value;
                }
            } else if(strncmp("ignore", setting->name, setting->name_len) == 0) {
                addToDynamicAstArray(&settings->ignore_list, &settings->ignore_count, &settings->ignore_capacity, setting->value);
            }
        }
    }
}
