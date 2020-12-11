
#include "parser.h"
#include "scanner.h"

#define PARSER_ERROR (void*)1

AstToken* parseToken(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstIdentifier* parseIdentifier(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstInlineC* parseInlineC(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstSequence* parseSequence(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstOption* parseOption(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstDefinition* parseDefinition(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

Ast* parseRootElement(Scanner* scanner, ErrorContext* error_context) {
    return NULL;
}

AstSetting* parseSetting(Scanner* scanner, ErrorContext* error_context) {
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
        freeAst(ret);
        addError(error_context, "Expected end of file", getOffsetOfNextToken(&scanner), ERROR);
        freeScanner(&scanner);
        freeAst((Ast*)ret);
        return NULL;
    } else {
        freeScanner(&scanner);
        return ret;
    }
}
