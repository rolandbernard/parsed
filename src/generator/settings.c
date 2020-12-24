
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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
                    addError(error_context, "Found multiple definitions of %args", setting->offset, ERROR);
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

void printArgNames(FILE* output, AstInlineC* args) {
    for (int i = 0; i < args->len; i++) {
        if (args->src[i] == ',') {
            int end = i;
            int paren = 0;
            while (end > 0 && (paren != 0 || !isalnum(args->src[end]))) {
                if (args->src[end] == ']') {
                    paren++;
                } else if (args->src[end] == '[') {
                    paren--;
                }
                end--;
            }
            end++;
            int start = end - 1;
            while (start > 0 && isalnum(args->src[start])) {
                start--;
            }
            start++;
            fwrite(args->src + start, 1, end - start, output);
            fputs(", ", output);
        }
    }
    int end = args->len;
    int paren = 0;
    while (end > 0 && (paren != 0 || !isalnum(args->src[end]))) {
        if (args->src[end] == ']') {
            paren++;
        } else if (args->src[end] == '[') {
            paren--;
        }
        end--;
    }
    end++;
    int start = end - 1;
    while (start > 0 && isalnum(args->src[start])) {
        start--;
    }
    start++;
    fwrite(args->src + start, 1, end - start, output);
}
