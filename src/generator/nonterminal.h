#ifndef _NONTERMINAL_H_
#define _NONTERMINAL_H_

typedef struct {
    const char* name;
    int name_len;
    int id;
} NonTerminal;

typedef struct {
    int count;
    int capacity;
    NonTerminal* data;
} NonTerminalTable;

void initNonTerminalTable(NonTerminalTable* table);

void freeNonTerminalTable(NonTerminalTable* table);

int addToNonTerminalTable(NonTerminalTable* table, NonTerminal non_terminal);

NonTerminal getFromNonTerminalTable(const NonTerminalTable* table, const char* name, int name_len);

void deleteFromNonTerminalTable(NonTerminalTable* table, const char* name, int name_len);

typedef void (*NonTerminalTableIterationFunction)(NonTerminal);

void forEachInNonTerminalTable(NonTerminalTable* table, NonTerminalTableIterationFunction func);

#endif