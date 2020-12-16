#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include <stdbool.h>

typedef struct {
    bool is_regex;
    const char* pattern;
    int pattern_len;
    int id;
} Terminal;

typedef struct {
    int count;
    int capacity;
    Terminal* data;
} TerminalTable;

void initTerminalTable(TerminalTable* table);

void freeTerminalTable(TerminalTable* table);

int addToTerminalTable(TerminalTable* table, Terminal terminal);

Terminal getFromTerminalTable(const TerminalTable* table, const char* pattern, int pattern_len, bool is_regex);

void deleteFromTerminalTable(TerminalTable* table, const char* pattern, int pattern_len, bool is_regex);

#endif