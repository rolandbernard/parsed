#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include <stdbool.h>

typedef struct {
    bool is_regex;
    bool is_code;
    const char* pattern;
    int pattern_len;
    int offset;
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

Terminal getFromTerminalTable(const TerminalTable* table, const char* pattern, int pattern_len, bool is_regex, bool is_code);

void deleteFromTerminalTable(TerminalTable* table, const char* pattern, int pattern_len, bool is_regex, bool is_code);

typedef void (*TerminalTableIterationFunction)(Terminal, void*);

void forEachInTerminalTable(TerminalTable* table, TerminalTableIterationFunction func, void* user_data);

#endif