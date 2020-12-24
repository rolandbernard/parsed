
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "parser.h"

typedef struct {
    GeneratorSettings* settings;
    FILE* output;
} SingleGenUserData;

static void generateSingleFunction(NonTerminal nonterminal, SingleGenUserData* data) {
    fputs("\nParsedToken* parsed_", data->output);
    fwrite(nonterminal.name, 1, nonterminal.name_len, data->output);
    fputs("(ParsedToken* parsed_tokens, ", data->output);
    if (data->settings->return_type == NULL) {
        fputs(" void** parsed_return", data->output);
    } else {
        fwrite(data->settings->return_type->src, 1, data->settings->return_type->len, data->output);
        fputs("* parsed_return", data->output);
    }
    if (data->settings->args != NULL) {
        fputs(", " , data->output);
        fwrite(data->settings->args->src, 1, data->settings->args->len, data->output);
    }
    fputs(");\n", data->output);
}

void generateParserFunctionDeclatations(FILE* output, NonTerminalTable* nonterminals, GeneratorSettings* settings) {
    SingleGenUserData data = {
        .output = output,
        .settings = settings,
    };
    forEachInNonTerminalTable(nonterminals, (NonTerminalTableIterationFunction)generateSingleFunction, (void*)&data);
}

static int compareSequences(AstSequence* s1, AstSequence* s2) {
    for (int i = 0; i < s1->child_count && i < s2->child_count; i++) {
        if(s1->children[i]->type != s2->children[i]->type) {
            if (s1->children[i]->type == AST_TOKEN) {
                return -1;
            } else {
                return 1;
            }
        } else if(s1->children[i]->type == AST_TOKEN) {
            AstToken* tk1 = (AstToken*)s1->children[i];
            AstToken* tk2 = (AstToken*)s2->children[i];
            if (tk1->id != tk2->id) {
                return tk1->id - tk2->id;
            }
        } else if(s1->children[i]->type == AST_IDENTIFIER) {
            AstIdentifier* iden1 = (AstIdentifier*)s1->children[i];
            AstIdentifier* iden2 = (AstIdentifier*)s2->children[i];
            if (iden1->id != iden2->id) {
                return iden1->id - iden2->id;
            }
        }
    }
    return s1->child_count - s2->child_count;
}

static void quickSortOptionSequences(AstSequence** sequences, int start, int end) {
    if (start + 2 == end) {
        if(compareSequences(sequences[start], sequences[start + 1]) > 0) {
            AstSequence* tmp = sequences[start];
            sequences[start] = sequences[start + 1];
            sequences[start + 1] = tmp;
        }
    } else if (start + 1 < end) {
        int order_start = start + 1;
        int order_end = end;
        int pivot_index = (start + end) / 2;
        AstSequence* tmp = sequences[start];
        sequences[start] = sequences[pivot_index];
        sequences[pivot_index] = tmp;
        AstSequence* pivot = sequences[start];
        while(order_start < order_end) {
            if(compareSequences(sequences[order_start], pivot) > 0) {
                order_end--;
                tmp = sequences[order_start];
                sequences[order_start] = sequences[order_end];
                sequences[order_end] = tmp;
            } else {
                order_start++;
            }
        }
        tmp = sequences[order_start - 1];
        sequences[order_start - 1] = sequences[start];
        sequences[start] = tmp;
        quickSortOptionSequences(sequences, start, order_start - 1);
        quickSortOptionSequences(sequences, order_end, end);
    }
}

static void recursivelyGenerateParser(
    FILE* output, AstSequence** sequences, int start, int end, int depth, int tab_depth,
    GeneratorSettings* settings, ErrorContext* error_context, const char* ret
) {
    char tabs[tab_depth + 1];
    for(int i = 0; i < tab_depth; i++) {
        tabs[i] = '\t';
    }
    tabs[tab_depth] = 0;
    int last_written = start;
    bool have_returns = false;
    bool have_terms = false;
    for (int i = start; i < end; i++) {
        if (sequences[i]->child_count > depth) {
            if (sequences[i]->children[depth]->type == AST_TOKEN) {
                have_terms = true;
            }
        } else if (sequences[i]->child_count == depth) {
            have_returns = true;
        }
    }
    if (have_terms) {
        fprintf(output, "%s\tswitch (parsed_current->kind) {\n", tabs);
        for (int i = start; i < end; i++) {
            if (sequences[i]->child_count > depth) {
                if (sequences[i]->children[depth]->type == AST_TOKEN) {
                    if (
                        i + 1 >= end || sequences[i + 1]->child_count <= depth
                        || sequences[i]->children[depth]->type != sequences[i + 1]->children[depth]->type
                        || ((AstToken*)sequences[i]->children[depth])->id != ((AstToken*)sequences[i + 1]->children[depth])->id
                    ) {
                        AstToken* tkn = (AstToken*)sequences[i]->children[depth];
                        fprintf(output, "%s\tcase %i:\n", tabs, tkn->id);
                        fprintf(output, "%s\t\tparsed_term[parsed_numterm] = *parsed_current;\n", tabs);
                        fprintf(output, "%s\t\tparsed_numterm++;\n", tabs);
                        fprintf(output, "%s\t\tparsed_current++;\n", tabs);
                        recursivelyGenerateParser(output, sequences, last_written, i + 1, depth + 1, tab_depth + 1, settings, error_context, ret);
                        fprintf(output, "%s\t\tparsed_numterm--;\n", tabs);
                        fprintf(output, "%s\t\tparsed_current--;\n", tabs);
                        fprintf(output, "%s\t\tbreak;\n", tabs);
                        last_written = i + 1;
                    }
                }
            }
        }
        fprintf(output, "%s\tdefault:\n", tabs);
        fprintf(output, "%s\t\tbreak;\n", tabs);
        fprintf(output, "%s\t}\n", tabs);
    }
    for (int i = start; i < end; i++) {
        if (sequences[i]->child_count > depth && sequences[i]->children[depth]->type == AST_IDENTIFIER) {
            if(
                i + 1 >= end || sequences[i + 1]->child_count <= depth
                || sequences[i]->children[depth]->type != sequences[i + 1]->children[depth]->type
                || ((AstIdentifier*)sequences[i]->children[depth])->id != ((AstIdentifier*)sequences[i + 1]->children[depth])->id
            ) {
                AstIdentifier* ident = (AstIdentifier*)sequences[i]->children[depth];
                fprintf(output, "%s\tif ((parsed_next = parsed_", tabs);
                fwrite(ident->ident, 1, ident->ident_len, output);
                fputs("(parsed_current, &parsed_nonterm[parsed_numnonterm]", output);
                if (settings->args != NULL) {
                    fputs(", ", output);
                    printArgNames(output, settings->args);
                }
                fputs(")) != NULL) {\n", output);
                fprintf(output, "%s\t\tparsed_numnonterm++;\n", tabs);
                fprintf(output, "%s\t\tParsedToken* parsed_temp = parsed_current;\n", tabs);
                fprintf(output, "%s\t\tparsed_current = parsed_next;\n", tabs);
                recursivelyGenerateParser(output, sequences, last_written, i + 1, depth + 1, tab_depth + 1, settings, error_context, ret);
                fprintf(output, "%s\t\tparsed_current = parsed_temp;\n", tabs);
                fprintf(output, "%s\t\tparsed_numnonterm--;\n", tabs);
                if (settings->free != NULL) {
                    AstInlineC* code = settings->free;
                    fprintf(output, "%s\t\t{\n", tabs);
                    int last_written = 0;
                    for (int j = 0; j < code->len; j++) {
                        if (code->src[j] == '$') {
                            fwrite(code->src + last_written, 1, j - last_written, output);
                            int len = 1;
                            while (isalnum(code->src[j + len]) || code->src[j + len] == '_') {
                                len++;
                            }
                            if (strncmp("$tofree", code->src + j, len) == 0) {
                                fputs(" parsed_nonterm[parsed_numnonterm] ", output);
                            } else {
                                addError(error_context, "Unknown parser variable", code->offset + 1 + j, ERROR);
                                fwrite(code->src + j, 1, len, output);
                            }
                            j += len - 1;
                            last_written = j + 1;
                        }
                    }
                    fwrite(code->src + last_written, 1, code->len - last_written, output);
                    fprintf(output, "\n%s\t\t}\n", tabs);
                }
                fprintf(output, "%s\t}\n", tabs);
                last_written = i + 1;
            }
        }
    }
    if (have_returns) {
        fprintf(output, "%s\t*parsed_return = parsed_nonterm[0];\n", tabs);
        for (int i = start; i < end; i++) {
            if (sequences[i]->child_count == depth) {
                if (sequences[i]->code != NULL) {
                    AstInlineC* code = sequences[i]->code;
                    fprintf(output, "%s\t{\n", tabs);
                    int last_written = 0;
                    for (int j = 0; j < code->len; j++) {
                        if (code->src[j] == '$') {
                            fwrite(code->src + last_written, 1, j - last_written, output);
                            int len = 1;
                            while (isalnum(code->src[j + len]) || code->src[j + len] == '_') {
                                len++;
                            }
                            if (strncmp("$numterm", code->src + j, len) == 0) {
                                fputs(" parsed_numterm ", output);
                            } else if (strncmp("$numnonterm", code->src + j, len) == 0) {
                                fputs(" parsed_numnonterm ", output);
                            } else if (strncmp("$term", code->src + j, len) == 0) {
                                fputs(" parsed_term ", output);
                            } else if (strncmp("$nonterm", code->src + j, len) == 0) {
                                fputs(" parsed_nonterm ", output);
                            } else if (strncmp("$return", code->src + j, len) == 0) {
                                fputs(" (*parsed_return) ", output);
                            } else {
                                addError(error_context, "Unknown parser variable", code->offset + 1 + j, ERROR);
                                fwrite(code->src + j, 1, len, output);
                            }
                            j += len - 1;
                            last_written = j + 1;
                        }
                    }
                    fwrite(code->src + last_written, 1, code->len - last_written, output);
                    fprintf(output, "\n%s\t}\n", tabs);
                }
            }
        }
        fprintf(output, "%s\t%s\n", tabs, ret);
    }
}


static void generateLeftRecursiveParser(FILE* output, AstOption* option, int id, GeneratorSettings* settings, ErrorContext* error_context) {
    int num_rec = 0;
    for (int i = 0; i < option->option_count; i++) {
        if (
            option->options[i]->child_count > 0 && option->options[i]->children[0]->type == AST_IDENTIFIER
            && ((AstIdentifier*)option->options[i]->children[0])->id == id
        ) {
            AstSequence* tmp = option->options[i];
            option->options[i] = option->options[num_rec];
            option->options[num_rec] = tmp;
            num_rec++;
        }
    }
    quickSortOptionSequences(option->options, 0, num_rec);
    quickSortOptionSequences(option->options, num_rec, option->option_count);
    recursivelyGenerateParser(output, option->options, num_rec, option->option_count, 0, 0, settings, error_context, "goto base_case_end;");
    fputs("\treturn NULL;\n", output);
    fputs("base_case_end:\n", output);
    fputs("\tfor (;;) {\n", output);
    fputs("\t\tparsed_nonterm[0] = *parsed_return;\n", output);
    fputs("\t\tparsed_numnonterm = 1;\n", output);
    fputs("\t\tparsed_numterm = 0;\n", output);
    recursivelyGenerateParser(output, option->options, 0, num_rec, 1, 1, settings, error_context, "goto new_recursive_expantion;");
    fputs("\t\treturn parsed_current;\n", output);
    fputs("\tnew_recursive_expantion:\n", output);
    fputs("\t\tcontinue;\n", output);
    fputs("\t}\n", output);
}

static void generateNonRecursiveParser(FILE* output, AstOption* option, int id, GeneratorSettings* settings, ErrorContext* error_context) {
    recursivelyGenerateParser(output, option->options, 0, option->option_count, 0, 0, settings, error_context, "return parsed_current;");
    fputs("\treturn NULL;\n", output);
}

void generateParser(FILE* output, Ast* ast, GeneratorSettings* settings, ErrorContext* error_context) {
    assert(ast->type == AST_ROOT);
    AstRoot* root = (AstRoot*)ast;
    for (int i = 0; i < root->child_count; i++) {
        if (root->children[i]->type == AST_DEFINITION) {
            AstDefinition* def = (AstDefinition*)root->children[i];
            fputs("\nParsedToken* parsed_", output);
            fwrite(def->ident->ident, 1, def->ident->ident_len, output);
            fputs("(ParsedToken* parsed_tokens, ", output);
            if (settings->return_type == NULL) {
                fputs(" void* parsed_return", output);
            } else {
                fwrite(settings->return_type->src, 1, settings->return_type->len, output);
                fputs("* parsed_return", output);
            }
            if (settings->args != NULL) {
                fputs(", ", output);
                fwrite(settings->args->src, 1, settings->args->len, output);
            }
            fputs(") {\n", output);
            bool is_left_recursive = false;
            bool is_recursive = false;
            bool has_base = false;
            for (int j = 0; !(is_left_recursive && is_recursive && has_base) && j < def->definition->option_count; j++) {
                AstSequence* option = (AstSequence*)def->definition->options[j];
                if (option->child_count > 0 && option->children[0]->type == AST_IDENTIFIER) {
                    AstIdentifier* first = (AstIdentifier*)option->children[0];
                    if (first->id == def->id) {
                        is_left_recursive = true;
                    }
                }
                bool is_seq_rec = false;
                for (int i = 0; !is_seq_rec && i < option->child_count; i++) {
                    if (option->children[i]->type == AST_IDENTIFIER) {
                        AstIdentifier* first = (AstIdentifier*)option->children[0];
                        if (first->id == def->id) {
                            is_seq_rec = true;
                        }
                    }
                }
                if (is_seq_rec) {
                    is_recursive = true;
                } else {
                    has_base = true;
                }
            }
            if (is_recursive && !has_base) {
                // Parsing of recursive definition without base case will never succed
                addError(error_context, "Missing recursive base case", def->offset, WARNING);
                fputs("\treturn NULL;\n", output);
            } else {
                quickSortOptionSequences(def->definition->options, 0, def->definition->option_count);
                int max_nonterms = 1;
                int max_terms = 1;
                for (int i = 0; i < def->definition->option_count; i++) {
                    if (i != 0 && compareSequences(def->definition->options[i - 1], def->definition->options[i]) == 0) {
                        addError(error_context, "Duplicate expantion options", def->definition->options[i]->offset, WARNING);
                    }
                    int num_nonterms = 0;
                    int num_terms = 0;
                    for (int j = 0; j < def->definition->options[i]->child_count; j++) {
                        if (def->definition->options[i]->children[j]->type == AST_TOKEN) {
                            num_terms++;
                        } else {
                            num_nonterms++;
                        }
                    }
                    if (num_terms > max_terms) {
                        max_terms = num_terms;
                    }
                    if (num_nonterms > max_nonterms) {
                        max_nonterms = num_nonterms;
                    }
                }
                fputs("\t", output);
                if (settings->return_type == NULL) {
                    fputs(" void* ", output);
                } else {
                    fwrite(settings->return_type->src, 1, settings->return_type->len, output);
                }
                fprintf(output, " parsed_nonterm[%i];\n", max_nonterms);
                fprintf(output, "\tParsedToken parsed_term[%i];\n", max_terms);
                fputs("\tint parsed_numnonterm = 0;\n", output);
                fputs("\tint parsed_numterm = 0;\n", output);
                fputs("\tParsedToken* parsed_current = parsed_tokens;\n", output);
                fputs("\tParsedToken* parsed_next = parsed_tokens;\n", output);
                if (is_left_recursive) {
                    generateLeftRecursiveParser(output, def->definition, def->id, settings, error_context);
                } else {
                    generateNonRecursiveParser(output, def->definition, def->id, settings, error_context);
                }
            }
            fputs("}\n", output);
        }
    }
}
