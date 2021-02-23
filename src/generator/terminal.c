
#include <stdlib.h>
#include <string.h>

#include "terminal.h"

#define DEFAULT_INITIAL_CAPACITY 8
#define DELETED (void*)1

void initTerminalTable(TerminalTable* table) {
    table->count = 0;
    table->capacity = 0;
    table->data = NULL;
}

void freeTerminalTable(TerminalTable* table) {
    free(table->data);
}

static unsigned long hashString(const char* str, int len, bool change_primes) {
    unsigned long hash = change_primes ? 5981 : 6151;
    for(int i = 0; i < len; i++) {
        hash = hash * (change_primes ? 811 : 769) + str[i];
    }
    return hash;
}

static void insertIntoData(Terminal* data, int capacity, Terminal token) {
    int index = hashString(token.pattern, token.pattern_len, token.is_regex || token.is_code) % capacity;
    while(data[index].pattern != NULL && data[index].pattern != DELETED) {
        index = (index + 1) % capacity;
    }
    data[index] = token;
}

static int findEntry(const Terminal* data, int capacity, const char* pattern, int pattern_len, bool is_regex, bool is_code) {
    if(capacity != 0) {
        int index = hashString(pattern, pattern_len, is_regex || is_code) % capacity;
        while (data[index].pattern != NULL) {
            if (
                data[index].pattern != DELETED
                && data[index].is_regex == is_regex
                && data[index].is_code == is_code
                && data[index].pattern_len == pattern_len
                && strncmp(data[index].pattern, pattern,  pattern_len) == 0
            ) {
                return index;
            }
            index = (index + 1) % capacity;
        }
    }
    return -1;
}

static void rehashHashTable(const Terminal* old_data, int old_capacity, Terminal* new_data, int new_capacity) {
    for(int i = 0; i < old_capacity; i++) {
        if(old_data[i].pattern != NULL && old_data[i].pattern != DELETED) {
            insertIntoData(new_data, new_capacity, old_data[i]);
        }
    }
}

static void checkForSizeGrowth(TerminalTable* table) {
    if (table->count * 3 >= table->capacity * 2) {
        if (table->capacity == 0) {
            table->capacity = 32;
            table->data = (Terminal*)calloc(table->capacity, sizeof(Terminal));
        } else {
            int new_capacity = table->capacity * 2;
            Terminal* new_data = (Terminal*)calloc(new_capacity, sizeof(Terminal));
            rehashHashTable(table->data, table->capacity, new_data, new_capacity);
            free(table->data);
            table->capacity = new_capacity;
            table->data = new_data;
        }
    }
}

static void checkForSizeShrink(TerminalTable* table) {
    if (table->count * 20 < table->capacity && table->count > 1) {
        int new_capacity = table->capacity / 2;
        Terminal* new_data = (Terminal*)calloc(new_capacity, sizeof(Terminal));
        rehashHashTable(table->data, table->capacity, new_data, new_capacity);
        free(table->data);
        table->capacity = new_capacity;
        table->data = new_data;
    }
}

int addToTerminalTable(TerminalTable* table, Terminal terminal) {
    checkForSizeGrowth(table);
    int index = findEntry(table->data, table->capacity, terminal.pattern, terminal.pattern_len, terminal.is_regex, terminal.is_code);
    if (index == -1) {
        terminal.id = table->count;
        insertIntoData(table->data, table->capacity, terminal);
        table->count++;
        return terminal.id;
    } else {
        return table->data[index].id;
    }
}

Terminal getFromTerminalTable(const TerminalTable* table, const char* pattern, int pattern_len, bool is_regex, bool is_code) {
    int index = findEntry(table->data, table->capacity, pattern, pattern_len, is_regex, is_code);
    if (index != -1) {
        return table->data[index];
    } else {
        Terminal ret = {
            .pattern = NULL,
            .pattern_len = 0,
            .is_regex = false,
            .is_code = false,
            .id = -1,
        };
        return ret;
    }
}

void deleteFromTerminalTable(TerminalTable* table, const char* pattern, int pattern_len, bool is_regex, bool is_code) {
    int index = findEntry(table->data, table->capacity, pattern, pattern_len, is_regex, is_code);
    if (index != -1) {
        table->data[index].pattern = DELETED;
        table->count--;
        checkForSizeShrink(table);
    }
}

void forEachInTerminalTable(TerminalTable* table, TerminalTableIterationFunction func, void* user_data) {
    for(int i = 0; i < table->capacity; i++) {
        if(table->data[i].pattern != NULL && table->data[i].pattern != DELETED) {
            func(table->data[i], user_data);
        }
    }
}

