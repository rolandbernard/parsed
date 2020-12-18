
#include <stdlib.h>
#include <stdio.h>

#include "errors.h"
#include "parser/parser.h"
#include "generator/generate.h"

char* loadFileContent(const char* filename, ErrorContext* error_context, int* size) {
    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        addErrorf(error_context, -1, ERROR, "Failed to open the file %s", filename);
        return NULL;
    } else {
        fseek(file, 0, SEEK_END);
        *size = ftell(file);
        fseek(file, 0, SEEK_SET);
        *size -= ftell(file);
        char* data = malloc(*size * sizeof(char));
        int read = fread(data, 1, *size, file);
        fclose(file);
        if(read != *size) {
            addErrorf(error_context, -1, ERROR, "Failed to read the file %s", filename);
            free(data);
            return NULL;
        } else {
            return data;
        }
    }
}

int main(int argc, const char* const* argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s [INPUT] [OUTPUT]\n", argv[0]);
        return EXIT_FAILURE;
    } else {
        ErrorContext errors;
        initErrorContext(&errors);
        int length;
        char* data = loadFileContent(argv[1], &errors, &length);
        if(data != NULL) {
            Ast* ast = parseGrammar(data, length, &errors);
            if(ast != NULL) {
                FILE* output = fopen(argv[2], "w");
                if(output == NULL) {
                    addErrorf(&errors, -1, ERROR, "Failed to open the file %s", argv[2]);
                } else {
                    generateLexerAndParser(output, ast, &errors);
                    fclose(output);
                }
                freeAst(ast);
            }
        }
        printErrors(stderr, &errors, data, argv[0]);
        if(data != NULL) {
            free(data);
        }
        int error_count = getErrorCount(&errors);
        freeErrorContext(&errors);
        if(error_count == 0) {
            return EXIT_SUCCESS;
        } else {
            return EXIT_FAILURE;
        }
    }
}
