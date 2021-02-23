#ifndef _AST_H_
#define _AST_H_

#include <stdbool.h>

#define AST_BASE AstType type; \
                 int offset;

typedef enum {
    AST_ROOT,
    AST_IDENTIFIER,
    AST_DEFINITION,
    AST_OPTION,
    AST_TOKEN,
    AST_INLINE_C,
    AST_SEQUENCE,
    AST_SETTING,
} AstType;

typedef struct {
    AST_BASE
} Ast;

typedef struct {
    AST_BASE
    Ast** children;
    int child_count;
    int child_capacity;
} AstRoot;

typedef struct {
    AST_BASE
    const char* ident;
    int ident_len;
    int id;
} AstIdentifier;

typedef struct {
    AST_BASE
    const char* src;
    int len;
    bool is_token;
    int id;
} AstInlineC;

typedef struct {
    AST_BASE
    Ast** children;
    int child_count;
    int child_capacity;
    AstInlineC* code;
} AstSequence;

typedef struct {
    AST_BASE
    AstSequence** options;
    int option_count;
    int option_capacity;
} AstOption;

typedef struct {
    AST_BASE
    AstIdentifier* ident;
    AstOption* definition;
    int id;
} AstDefinition;

typedef struct {
    AST_BASE
    bool is_regex;
    const char* pattern;
    int pattern_len;
    int id;
} AstToken;

typedef struct {
    AST_BASE
    const char* name;
    int name_len;
    Ast* value;
} AstSetting;

void freeAst(Ast* ast);

AstRoot* createAstRoot();

void addChildToAstRoot(AstRoot* root, Ast* child);

AstIdentifier* createAstIdentifier(const char* str, int len, int offset);

AstDefinition* createAstDefinition(AstIdentifier* ident, AstOption* def, int offset);

AstOption* createAstOption();

void addOptionToAstOption(AstOption* ast, AstSequence* child);

AstToken* createAstToken(bool is_regex, const char* str, int len, int offset);

AstInlineC* createAstInlineC(const char* str, int len, int offset, bool is_token);

AstSequence* createAstSequence();

void addChildToAstSequence(AstSequence* seq, Ast* child);

AstSetting* createAstSetting(const char* name, int name_len, Ast* value, int offset);

void addToDynamicAstArray(Ast*** data, int* count, int* capacity, Ast* value);

#endif