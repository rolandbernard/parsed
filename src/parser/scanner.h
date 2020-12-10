#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <stdbool.h>

typedef enum {
    TOKEN_INVALID,
    TOKEN_IDENTIFIER,
    TOKEN_STRING_TOKEN,
    TOKEN_REGEX_TOKEN,
    TOKEN_DEFINE,
    TOKEN_ALTERNATIVE,
    TOKEN_C_SOURCE,
    TOKEN_PERCENT,
    TOKEN_UNCLOSED_STRING,
    TOKEN_UNCLOSED_REGEX,
    TOKEN_UNCLOSED_C_SOURCE,
    TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int offset;
    int len;
} Token;

typedef struct {
    const char* src;
    int offset;
    bool is_cached;
    Token cached;
} Scanner;

void initScanner(Scanner* scanner, const char* src);

Token getNextToken(Scanner* scanner);

Token consumeNextToken(Scanner* scanner);

bool acceptToken(Scanner* scanner, TokenType type, Token* out);

int getOffsetOfNextToken(Scanner* scanner);

void freeScanner(Scanner* scanner);

#endif