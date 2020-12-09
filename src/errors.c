
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>

#include "errors.h"

#define MAX_ERROR_FORMAT 1024

void initErrorContext(ErrorContext* context) {
    context->error_capacity = 0;
    context->error_count = 0;
    context->errors = NULL;
}

void freeErrorContext(ErrorContext* context) {
    for (int e = 0; e < context->error_count; e++) {
        free(context->errors[e].message);
    }
    free(context->errors);
    initErrorContext(context);
}

void addError(ErrorContext* context, const char* message, int offset, ErrorLevel level) {
    if(context != NULL) {
        if (context->error_capacity == context->error_count) {
            context->error_capacity = context->error_capacity * 2 + 4;
            context->errors = (Error*)realloc(context->errors, sizeof(Error) * context->error_capacity);
        }
        int len = strlen(message);
        char* msg = (char*)malloc(len + 1);
        memcpy(msg, message, len + 1);
        context->errors[context->error_count].message = msg;
        context->errors[context->error_count].offset = offset;
        context->errors[context->error_count].level = level;
        context->error_count++;
    }
}

typedef struct {
    int line;
    int column;
    const char* line_start;
    int line_len;
} LineInfo;

static LineInfo getLineInfo(int offset, const char* file_content) {
    const char* last_line = file_content;
    int line = 1;
    int column = 1;
    int i;
    for(i = 0; i < offset; i++) {
        if(file_content[i] == '\n') {
            last_line = file_content + i + 1;
            line++;
            column = 1;
        } else {
            column++;
        }
    }
    while(file_content[i] != '\n' && file_content[i] != 0) {
        i++;
    }
    LineInfo ret = {
        .line = line,
        .column = column,
        .line_start = last_line,
        .line_len = (int)((file_content + i) - last_line),
    };
    return ret;
}

static const char* error_level_tty[] = {
    [ERROR]   = "\e[31merror\e[m",
    [WARNING] = "\e[35mwarning\e[m",
    [NOTE]    = "\e[36mnote\e[m",
};

static const char* error_level_notty[] = {
    [ERROR]   = "error",
    [WARNING] = "warning",
    [NOTE]    = "note",
};

void printErrors(FILE* file, ErrorContext* context, const char* file_content, const char* filename) {
    bool istty = isatty(fileno(file));
    const char** error_level = error_level_notty;
    if(istty) {
        error_level = error_level_tty;
    }
    for(int i = 0; i < context->error_count; i++) {
        if(context->errors[i].offset != -1) {
            LineInfo info = getLineInfo(context->errors[i].offset, file_content);
            fprintf(file, "%s: %s:%i:%i: %s\n", error_level[context->errors[i].level], filename, info.line, info.column, context->errors[i].message);
            int printed = fprintf(file, "%5i", info.line);
            fprintf(file, " | ");
            fwrite(info.line_start, 1, info.line_len, file);
            putc('\n', file);
            for (int j = 0; j < printed; j++) {
                putc(' ', file);
            }
            fprintf(file, " |");
            for (int j = 0; j < info.column; j++) {
                if(info.line_start[j] == '\t') {
                    putc('\t', file);
                } else {
                    putc(' ', file);
                }
            }
            if(istty) {
                fprintf(file, "\e[31m^\e[m\n");
            } else {
                fprintf(file, "^\n");
            }
        } else {
            fprintf(file, "%s: %s\n", error_level[context->errors[i].level], context->errors[i].message);
        }
    }
}

int getErrorCount(ErrorContext* context) {
    int ret = 0;
    for(int i = 0; i < context->error_count; i++) {
        if(context->errors[i].level == ERROR) {
            ret++;
        }
    }
    return ret;
}

void addErrorf(ErrorContext* context, int offset, ErrorLevel level, const char* format, ...) {
    char msg[MAX_ERROR_FORMAT];
    va_list varargs;
    va_start(varargs, format);
    vsnprintf(msg, MAX_ERROR_FORMAT, format, varargs);
    va_end(varargs);
    addError(context, msg, offset, level);
}