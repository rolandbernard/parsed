
#include "parser.h"
#include "scanner.h"

#define PARSER_ERROR (void*)1

AstToken* parseToken(Scanner* scanner, ErrorContext* error_context) {
    Token token;
    if(acceptToken(scanner, TOKEN_STRING_TOKEN, &token)) {
        return createAstToken(false, token.start + 1, token.len - 2, token.offset);
    } else if(acceptToken(scanner, TOKEN_REGEX_TOKEN, &token)) {
        return createAstToken(true, token.start + 1, token.len - 2, token.offset);
    } else {
        return NULL;
    }
}

AstIdentifier* parseIdentifier(Scanner* scanner, ErrorContext* error_context) {
    Token token;
    if(acceptToken(scanner, TOKEN_IDENTIFIER, &token)) {
        return createAstIdentifier(token.start, token.len, token.offset);
    } else {
        return NULL;
    }
}

AstInlineC* parseInlineC(Scanner* scanner, ErrorContext* error_context) {
    Token token;
    if(acceptToken(scanner, TOKEN_C_SOURCE, &token)) {
        return createAstInlineC(token.start + 1, token.len - 2, token.offset);
    } else {
        return NULL;
    }
}

AstSequence* parseSequence(Scanner* scanner, ErrorContext* error_context) {
    AstSequence* ret = createAstSequence();
    Ast* elem;
    while((elem = (Ast*)parseToken(scanner, error_context)) != NULL || (elem = (Ast*)parseIdentifier(scanner, error_context)) != NULL) {
        if(elem == PARSER_ERROR) {
            freeAst((Ast*)ret);
            return PARSER_ERROR;
        } else {
            addChildToAstSequence(ret, elem);
        }
    }
    AstInlineC* code = parseInlineC(scanner, error_context);
    if(code != NULL) {
        if(code == PARSER_ERROR) {
            freeAst((Ast*)ret);
            return PARSER_ERROR;
        } else {
            ret->code = code;
        }
    }
    if(ret->child_count == 0) {
        ret->offset = getOffsetOfNextToken(scanner) - 1;
        return ret;
    } else {
        ret->offset = ret->children[0]->offset;
        return ret;
    }
}

AstOption* parseOption(Scanner* scanner, ErrorContext* error_context) {
    AstOption* ret = createAstOption();
    AstSequence* elem;
    while((ret->option_count == 0 || acceptToken(scanner, TOKEN_ALTERNATIVE, NULL))) {
        elem = parseSequence(scanner, error_context);
        if(elem == NULL) {
            if(ret->option_count == 0) {
                freeAst((Ast*)ret);
                return NULL;
            } else {
                freeAst((Ast*)ret);
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
    AstIdentifier* ident = parseIdentifier(scanner, error_context);
    if(ident == PARSER_ERROR) {
        return PARSER_ERROR;
    } else if(ident == NULL) {
        return NULL;
    } else {
        if(acceptToken(scanner, TOKEN_DEFINE, NULL)) {
            AstOption* definition = parseOption(scanner, error_context);
            if(definition == PARSER_ERROR || definition == NULL) {
                freeAst((Ast*)ident);
                if(definition == NULL) {
                    addError(error_context, "Expected a definition", getOffsetOfNextToken(scanner), ERROR);
                }
                return PARSER_ERROR;
            } else if(!acceptToken(scanner, TOKEN_SEMICOLON, NULL)) {
                addError(error_context, "Expected a semicolon", getOffsetOfNextToken(scanner), ERROR);
                freeAst((Ast*)ident);
                freeAst((Ast*)definition);
                return PARSER_ERROR;
            } else {
                return createAstDefinition(ident, definition, ident->offset);
            }
        } else {
            freeAst((Ast*)ident);
            addError(error_context, "Expected a ':='", getOffsetOfNextToken(scanner), ERROR);
            return PARSER_ERROR;
        }
    }
}

Ast* parseGlobalInlineCOrSetting(Scanner* scanner, ErrorContext* error_context) {
    Token first;
    if(acceptToken(scanner, TOKEN_PERCENT, &first)) {
        Token next = getNextToken(scanner);
        if(next.type == TOKEN_C_SOURCE) {
            acceptToken(scanner, TOKEN_C_SOURCE, NULL);
            return (Ast*)createAstInlineC(next.start + 1, next.len - 2, first.offset);
        } else if(next.type == TOKEN_IDENTIFIER) {
            acceptToken(scanner, TOKEN_IDENTIFIER, NULL);
            Ast* value = (Ast*)parseToken(scanner, error_context);
            if(value == NULL) {
                value = (Ast*)parseInlineC(scanner, error_context);
            }
            if(value == PARSER_ERROR) {
                return PARSER_ERROR;
            } else if(value == NULL) {
                addError(error_context, "Expected a settings value (Token or C code)", getOffsetOfNextToken(scanner), ERROR);
                return PARSER_ERROR;
            } else {
                return (Ast*)createAstSetting(next.start, next.len, value, first.offset);
            }
        } else {
            addError(error_context, "Expected a setting name or c code", getOffsetOfNextToken(scanner), ERROR);
            return PARSER_ERROR;
        }
    } else {
        return NULL;
    }
}

Ast* parseRootElement(Scanner* scanner, ErrorContext* error_context) {
    Ast* ret = (Ast*)parseGlobalInlineCOrSetting(scanner, error_context);
    if(ret == NULL) {
        ret = (Ast*)parseDefinition(scanner, error_context);
    }
    return ret;
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

Ast* parseGrammar(const char* src, int len, ErrorContext* error_context) {
    Scanner scanner;
    initScanner(&scanner, src, len);
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
        freeAst((Ast*)ret);
        freeScanner(&scanner);
        return NULL;
    } else {
        freeScanner(&scanner);
        return (Ast*)ret;
    }
}
