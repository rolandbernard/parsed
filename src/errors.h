#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <stdio.h>

typedef enum {
    ERROR,
    WARNING,
    NOTE,
} ErrorLevel;

typedef struct {
    char* message;
    int offset;
    ErrorLevel level;
} Error;

typedef struct {
    int error_count;
    int error_capacity;
    Error* errors;
} ErrorContext;

void initErrorContext(ErrorContext* context);

void freeErrorContext(ErrorContext* context);

void addError(ErrorContext* context, const char* message, int offset, ErrorLevel level);

void addErrorf(ErrorContext* context, int offset, ErrorLevel level, const char* messag, ...);

void printErrors(FILE* file, ErrorContext* context, const char* file_content);

int getErrorCount(ErrorContext* context);

#endif