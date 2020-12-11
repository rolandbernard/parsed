
#include "parser.h"
#include "scanner.h"

#define PARSER_ERROR (void*)1

AstToken* parseToken(Scanner* scanner, ErrorContext* error_context) {
    Token token;
    if(acceptToken(scanner, TOKEN_STRING_TOKEN, &token)) {
        return createAstToken(false, token.start + 1, token.len - 2);
    } else if(acceptToken(scanner, TOKEN_REGEX_TOKEN, &token)) {
        return createAstToken(true, token.start + 1, token.len - 2);
    } else {
        return NULL;
    }
}

AstIdentifier* parseIdentifier(Scanner* scanner, ErrorContext* error_context) {
    Token token;
    if(acceptToken(scanner, TOKEN_IDENTIFIER, &token)) {
        return createAstIdentifier(token.start, token.len);
    } else {
        return NULL;
    }
}

AstInlineC* parseInlineC(Scanner* scanner, ErrorContext* error_context) {
    Token token;
    if(acceptToken(scanner, TOKEN_C_SOURCE, &token)) {
        return createAstInlineC(token.start + 1, token.len - 2);
    } else {
        return NULL;
    }
}

AstSequence* parseSequence(Scanner* scanner, ErrorContext* error_context) {
    AstSequence* ret = createAstSequence();
    Ast* elem;
    while((elem = parseToken(scanner, error_context)) != NULL) {
        if(elem == PARSER_ERROR) {
            freeAst((Ast*)ret);
            return PARSER_ERROR;
        } else {
            addChildToAstSequence(ret, elem);
        }
    }
    if(ret->child_count == 0) {
        freeAst((Ast*)ret);
        return NULL;
    } else {
        elem = parseInlineC(scanner, error_context);
        if(elem != NULL) {
            if(elem == PARSER_ERROR) {
                freeAst((Ast*)ret);
                return PARSER_ERROR;
            } else {
                addChildToAstSequence(ret, elem);
            }
        }
        return ret;
    }
}

AstOption* parseOption(Scanner* scanner, ErrorContext* error_context) {
    AstOption* ret = createAstOption();
    Ast* elem;
    while((ret->option_count == 0 || acceptToken(scanner, TOKEN_ALTERNATIVE, NULL))) {
        elem = parseSequence(scanner, error_context);
        if(elem == NULL) {
            freeAst((Ast*)ret);
            if(ret->option_count == 0) {
                return NULL;
            } else {
                addError(error_context, "Expected an expantions", getOffsetOfNextToken(scanner), ERROR);
                return PARSER_ERROR;
            }
        } else if(elem == PARSER_ERROR) {
            freeAst((Ast*)ret);
            return PARSER_ERROR;
        } else {
            addOptionToAstOption(ret, elem);
        }
    }
    return ret;
}

AstDefinition* parseDefinition(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstInlineC* parseGlobalInlineC(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstSetting* parseSetting(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

Ast* parseRootElement(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstRoot* parseRoot(Scanner* scanner, ErrorContext* error_context) {
    AstRoot* ret = createAstRoot();
    Ast* child = NULL;
    while((child = parseRootElement(scanner, error_context)) != NULL) {
        if(child == PARSER_ERROR) {
            freeAst((Ast*)ret);
            return PARSER_ERROR;
        } else {
            addChildToAstRoot(ret, child);
        }
    }
    return ret;
}

Ast* parseGrammar(const char* src, ErrorContext* error_context) {
    Scanner scanner;
    initScanner(&scanner, src);
    AstRoot* ret = parseRoot(&scanner, error_context);
    if(ret == PARSER_ERROR) {
        freeScanner(&scanner);
        return NULL;
    } else if(!acceptToken(&scanner, TOKEN_EOF, NULL)) {
        Token next = getNextToken(&scanner);
        if(next.type == TOKEN_INVALID) {
            addError(error_context, "Found an invalid token", getOffsetOfNextToken(&scanner), ERROR);
        } else if(next.type == TOKEN_UNCLOSED_STRING) {
            addError(error_context, "Found an unclosed string", getOffsetOfNextToken(&scanner), ERROR);
        } else if(next.type == TOKEN_UNCLOSED_REGEX) {
            addError(error_context, "Found an unclosed regex", getOffsetOfNextToken(&scanner), ERROR);
        } else if(next.type == TOKEN_UNCLOSED_C_SOURCE) {
            addError(error_context, "Found an unclosed inline c block", getOffsetOfNextToken(&scanner), ERROR);
        } else {
            addError(error_context, "Expected end of file", getOffsetOfNextToken(&scanner), ERROR);
        }
        freeAst(ret);
        freeScanner(&scanner);
        freeAst((Ast*)ret);
        return NULL;
    } else {
        freeScanner(&scanner);
        return ret;
    }
}
