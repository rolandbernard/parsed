
#include <ctype.h>
#include <stddef.h>

#include "scanner.h"

void initScanner(Scanner* scanner, const char* src) {
    scanner->src = src;
    scanner->offset = 0;
    scanner->is_cached = false;
}

static int determenNextToken(const char* src, TokenType* out) {
    int len = 1;
    *out = TOKEN_INVALID;
    if(isalnum(src[0]) || src[0] == '_') {
        while (isalnum(src[len]) || src[len] == '_') {
            len++;
        }
        *out = TOKEN_IDENTIFIER;
    } else if(src[0] == '{') {
        int nested = 0;
        while (src[len] != 0 && (nested != 0 || src[len] != '}')) {
            if(src[len] == '{') {
                nested++;
            } else if(src[len] == '}') {
                nested--;
            }
            len++;
        }
        if(src[len] == 0) {
            *out = TOKEN_UNCLOSED_C_SOURCE;
        } else {
            len++;
            *out = TOKEN_C_SOURCE;
        }
    } else if(src[0] == '"' || src[0] == '\'') {
        while (src[len] != 0 && src[len] != src[0]) {
            if(src[len] == '\\') {
                len++;
            }
            len++;
        }
        if(src[len] == 0) {
            *out = TOKEN_UNCLOSED_STRING;
        } else {
            len++;
            *out = TOKEN_STRING_TOKEN;
        }
    } else if(src[0] == '/') {
        while (src[len] != 0 && src[len] != '/') {
            if(src[len] == '\\') {
                len++;
            }
            len++;
        }
        if(src[len] == 0) {
            *out = TOKEN_UNCLOSED_REGEX;
        } else {
            len++;
            *out = TOKEN_REGEX_TOKEN;
        }
    } else if(src[0] == '%') {
        *out = TOKEN_PERCENT;
    } else if(src[0] == '|') {
        *out = TOKEN_ALTERNATIVE;
    } else if(src[0] == ':' && src[1] == '=') {
        len = 2;
        *out = TOKEN_DEFINE;
    } else if(src[0] == 0) {
        len = 0;
        *out = TOKEN_EOF;
    }
    return len;
}

Token getNextToken(Scanner* scanner) {
    if(!scanner->is_cached) {
        TokenType type;
        int len = determenNextToken(scanner->src + scanner->offset, &type);
        scanner->cached.start = scanner->src + scanner->offset;
        scanner->cached.len = len;
        scanner->cached.offset = scanner->offset;
        scanner->cached.type = type;
        scanner->offset += len;
        scanner->is_cached = true;
    }
    return scanner->cached;
}

Token consumeNextToken(Scanner* scanner) {
    getNextToken(scanner);
    scanner->is_cached = false;
    return scanner->cached;
}

bool acceptToken(Scanner* scanner, TokenType type, Token* out) {
    getNextToken(scanner);
    if(out != NULL) {
        *out = scanner->cached;
    }
    if(scanner->cached.type == type) {
        scanner->is_cached = false;
        return true;
    } else {
        return false;
    }
}

int getOffsetOfNextToken(Scanner* scanner) {
    return getNextToken(scanner).offset;
}

void freeScanner(Scanner* scanner) {
    // Nothing to free
}
