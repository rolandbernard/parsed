
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

static void insertIntoData(Terminal* data, int capacity, const char* name, int name_len, int id, bool is_regex) {
    int index = hashString(name, name_len, is_regex) % capacity;
    while(data[index].name != NULL && data[index].name != DELETED) {
        index = (index + 1) % capacity;
    }
    data[index].name = name;
    data[index].name_len = name_len;
    data[index].is_regex = is_regex;
    data[index].id = id;
}

static int findEntry(const Terminal* data, int capacity, const char* name, int name_len, bool is_regex) {
    if(capacity != 0) {
        int index = hashString(name, name_len, is_regex) % capacity;
        while (data[index].name != NULL) {
            if (
                data[index].name != DELETED
                && data[index].is_regex == is_regex
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

static void rehashHashTable(const Terminal* old_data, int old_capacity, const Terminal* new_data, int new_capacity) {
    for(int i = 0; i < old_capacity; i++) {
        if(old_data[i].name != NULL && old_data[i].name != DELETED) {
            insertIntoData(new_data, new_capacity, old_data[i].name, old_data[i].name_len, old_data[i].id, old_data[i].is_regex);
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

int addToNonTerminalTable(TerminalTable* table, Terminal non_terminal) {
    checkForSizeGrowth(table);
    int index = findEntry(table->data, table->capacity, non_terminal.name, non_terminal.name_len, non_terminal.is_regex);
    if (index == -1) {
        insertIntoData(table->data, table->capacity, non_terminal.name, non_terminal.name_len, non_terminal.id || (table->count + 1), non_terminal.is_regex);
        table->count++;
        return table->count;
    } else {
        return table->data[index].id;
    }
}

Terminal getFromNonTerminalTable(const TerminalTable* table, const char* name, int name_len, bool is_regex) {
    int index = findEntry(table->data, table->capacity, name, name_len, is_regex);
    if (index != -1) {
        return table->data[index];
    } else {
        Terminal ret = {
            .name = NULL,
            .name_len = 0,
            .is_regex = false,
            .id = 0,
        };
        return ret;
    }
}

void deleteFromNonTerminalTable(TerminalTable* table, const char* name, int name_len, bool is_regex) {
    int index = findEntry(table->data, table->capacity, name, name_len, is_regex);
    if (index != -1) {
        table->data[index].name = DELETED;
        table->count--;
        checkForSizeShrink(table);
    }
}