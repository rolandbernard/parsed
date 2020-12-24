
#include <stdlib.h>
#include <string.h>

#include "nonterminal.h"

#define DEFAULT_INITIAL_CAPACITY 8
#define DELETED (void*)1

void initNonTerminalTable(NonTerminalTable* table) {
    table->count = 0;
    table->capacity = 0;
    table->data = NULL;
}

void freeNonTerminalTable(NonTerminalTable* table) {
    free(table->data);
}

static unsigned long hashString(const char* str, int len) {
    unsigned long hash = 6151;
    for(int i = 0; i < len; i++) {
        hash = hash * 769 + str[i];
    }
    return hash;
}

static void insertIntoData(NonTerminal* data, int capacity, NonTerminal nonterm) {
    int index = hashString(nonterm.name, nonterm.name_len) % capacity;
    while(data[index].name != NULL && data[index].name != DELETED) {
        index = (index + 1) % capacity;
    }
    data[index] = nonterm;
}

static int findEntry(const NonTerminal* data, int capacity, const char* name, int name_len) {
    if(capacity != 0) {
        int index = hashString(name, name_len) % capacity;
        while (data[index].name != NULL) {
            if (
                data[index].name != DELETED
                && data[index].name_len == name_len
                && strncmp(data[index].name, name,  name_len) == 0
            ) {
                return index;
            }
            index = (index + 1) % capacity;
        }
    }
    return -1;
}

static void rehashHashTable(const NonTerminal* old_data, int old_capacity, NonTerminal* new_data, int new_capacity) {
    for(int i = 0; i < old_capacity; i++) {
        if(old_data[i].name != NULL && old_data[i].name != DELETED) {
            insertIntoData(new_data, new_capacity, old_data[i]);
        }
    }
}

static void checkForSizeGrowth(NonTerminalTable* table) {
    if (table->count * 3 >= table->capacity * 2) {
        if (table->capacity == 0) {
            table->capacity = 32;
            table->data = (NonTerminal*)calloc(table->capacity, sizeof(NonTerminal));
        } else {
            int new_capacity = table->capacity * 2;
            NonTerminal* new_data = (NonTerminal*)calloc(new_capacity, sizeof(NonTerminal));
            rehashHashTable(table->data, table->capacity, new_data, new_capacity);
            free(table->data);
            table->capacity = new_capacity;
            table->data = new_data;
        }
    }
}

static void checkForSizeShrink(NonTerminalTable* table) {
    if (table->count * 20 < table->capacity && table->count > 1) {
        int new_capacity = table->capacity / 2;
        NonTerminal* new_data = (NonTerminal*)calloc(new_capacity, sizeof(NonTerminal));
        rehashHashTable(table->data, table->capacity, new_data, new_capacity);
        free(table->data);
        table->capacity = new_capacity;
        table->data = new_data;
    }
}

int addToNonTerminalTable(NonTerminalTable* table, NonTerminal non_terminal) {
    checkForSizeGrowth(table);
    int index = findEntry(table->data, table->capacity, non_terminal.name, non_terminal.name_len);
    if (index == -1) {
        non_terminal.id = table->count;
        insertIntoData(table->data, table->capacity, non_terminal);
        table->count++;
        return non_terminal.id;
    } else {
        table->data[index].defined = table->data[index].defined || non_terminal.defined;
        return table->data[index].id;
    }
}

NonTerminal getFromNonTerminalTable(const NonTerminalTable* table, const char* name, int name_len) {
    int index = findEntry(table->data, table->capacity, name, name_len);
    if (index != -1) {
        return table->data[index];
    } else {
        NonTerminal ret = {
            .name = NULL,
            .name_len = 0,
            .id = -1,
            .defined = false,
        };
        return ret;
    }
}

void deleteFromNonTerminalTable(NonTerminalTable* table, const char* name, int name_len) {
    int index = findEntry(table->data, table->capacity, name, name_len);
    if (index != -1) {
        table->data[index].name = DELETED;
        table->count--;
        checkForSizeShrink(table);
    }
}

void forEachInNonTerminalTable(NonTerminalTable* table, NonTerminalTableIterationFunction func, void* user_data) {
    for(int i = 0; i < table->capacity; i++) {
        if(table->data[i].name != NULL && table->data[i].name != DELETED) {
            func(table->data[i], user_data);
        }
    }
}
