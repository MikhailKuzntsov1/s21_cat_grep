#include "s21_grep.h"

#include "../common/s21_common.h"

int main(int argc, const char **argv) {
    if (argc < 3)
        printf("ERROR! Too few arguments for grep call!\n");

    if (argc >= 3) {
        int flagset = 0;
        parse_grep_flags(argv, &flagset, argc);

        char **files = calloc(argc, sizeof(char *));
        char **pfiles = calloc(argc, sizeof(char *));
        char **inline_patterns = calloc(argc, sizeof(char *));
        int *busy_ind = calloc(argc, sizeof(int));
        reg_node_t *root = NULL;

        if (files && pfiles && busy_ind && inline_patterns) {
            grep_argv_parser(argv, argc, flagset, pfiles, busy_ind, inline_patterns, files);

            int fail = 0;  // non existing pfile fail
            root = create_list_of_regexes(pfiles, inline_patterns, flagset, &fail);

#ifdef DEBUG
            print_debug(flagset, files, pfiles, inline_patterns);
            print_regex_list(root);
#endif

            if (!fail)
                grep(root, files, flagset);
        }

        free_existing_memory(files, pfiles, inline_patterns, busy_ind, root);
    }

    return EXIT_SUCCESS;
}

void free_existing_memory(char **p1, char **p2, char **p3, int *p4, reg_node_t *p5) {
    if (p1) free(p1);
    if (p2) free(p2);
    if (p3) free(p3);
    if (p4) free(p4);
    if (p5) destroy_regex_list(p5);
}

void grep_argv_parser(const char **argv, int argc,
                      int flagset, char **pfiles, int *busy_ind, char **inline_patterns, char **files) {
    if (IS_SET(flagset, REGEX_FL))
        find_inline_patterns(argc, (const char **)argv, busy_ind, inline_patterns);
    if (IS_SET(flagset, FILE_REGEX_FL))
        find_pfiles(argc, argv, pfiles, busy_ind);

    find_files(argc, argv, busy_ind, files);

    if (!IS_SET(flagset, REGEX_FL) && !IS_SET(flagset, FILE_REGEX_FL)) {
        handle_no_pattern_input(files, busy_ind, inline_patterns, argc, argv);
    }
}

void resolve_flag_conflicts(int *flags) {
    /* According to Andrey (@capricey), -v -o flags make grep behave as if only -v was given */
    /* -l -c flags also turn off -o */
    if (IS_SET(*flags, INVERSE_FL) ||
        IS_SET(*flags, MATCH_COUNT_FL) ||
        IS_SET(*flags, FILE_FL)) {
        CLEAR_BIT(*flags, ONLY_MATCHED_PARTS_FL);
    }
}

/* Separating business logic and IO tasks */
int get_line_from_file(char **files, int flagset, struct grep_config *c) {
    int res = 0;
    size_t linecap = 2;

    static FILE *fd = NULL;

    if (!fd)
        fd = fopen(files[c->cur_file], "r");

    if (!fd) {
        if (!IS_SET(flagset, SUPPRESS_ERR_FL))
            perror("");
        return 0;
    } else {
        /* VERY evil line! Operation '=' returns positive value in case of success */
        /* It will fail when we will try to make res EOF */
        ssize_t line_len;
        res = ((line_len = getline(&(c->line), &linecap, fd)) > 0);

        if (!res) {
            fclose(fd);
            fd = NULL;
        } else {
            c->non_empty = 1;
            c->cur_line++;
        }
    }

    return res;
}

void handle_grep_output_no_o(char **files, int flagset, struct grep_config *c) {
    if (!IS_SET(flagset, ONLY_MATCHED_PARTS_FL)) {
        if (c->files_n > 1 && !IS_SET(flagset, SUPPRESS_MUL_FNAME_FL) &&
            !IS_SET(flagset, MATCH_COUNT_FL) && !IS_SET(flagset, FILE_FL)) {
            fprintf(stdout, "%s:", files[c->cur_file]);
        }

        if (IS_SET(flagset, LINE_N_FL) && !IS_SET(flagset, MATCH_COUNT_FL) &&
            !IS_SET(flagset, FILE_FL))
            fprintf(stdout, "%d:", c->cur_line);

        if (!IS_SET(flagset, MATCH_COUNT_FL) && !IS_SET(flagset, FILE_FL)) {
            fprintf(stdout, "%s", c->line);
            if (c->line[strlen(c->line) - 1] != '\n')
                fprintf(stdout, "\n");
        }
    }
}

void handle_grep_post_flags(struct grep_config *conf, char **files, int flagset) {
    if (IS_SET(flagset, MATCH_COUNT_FL) && conf->non_empty) {
        if (conf->files_n > 1 && !IS_SET(flagset, SUPPRESS_MUL_FNAME_FL))
            fprintf(stdout, "%s:", files[conf->cur_file]);

        if (IS_SET(flagset, FILE_FL) && conf->total_matches > 1)
            conf->total_matches = 1;

        fprintf(stdout, "%d\n", conf->total_matches);
    }

    if (IS_SET(flagset, FILE_FL) && conf->total_matches > 0) {
        fprintf(stdout, "%s\n", files[conf->cur_file]);
    }
    conf->cur_file++;
}

void grep(reg_node_t *root, char **files, int flagset) {
    struct grep_config conf = {
        .files_n = count_files(files),
        .cur_file = 0,
        .total_matches = 0,
        .line = NULL,
    };

    reset_config(&conf);

    if (files[0] == NULL)
        conf.cur_file = 1;

    while (files[conf.cur_file]) {
        while (get_line_from_file(files, flagset, &conf)) {
            if (!IS_SET(flagset, ONLY_MATCHED_PARTS_FL)) {
                int res = match_regex_ll(root, conf.line, flagset, conf.match);
                if (!res) {
                    conf.total_matches++;
                    handle_grep_output_no_o(files, flagset, &conf);
                }
            } else {
                handle_o_output(root, files, flagset, &conf);
            }
        }
        if (!IS_SET(flagset, ONLY_MATCHED_PARTS_FL))
            handle_grep_post_flags(&conf, files, flagset);
        else
            conf.cur_file++;

        reset_config(&conf);
    }

    if (conf.line)
        free(conf.line);
}

void reset_config(struct grep_config *c) {
    c->total_matches = 0;
    c->cur_line = 0;
    c->non_empty = 0;
}

void handle_o_output(reg_node_t *root, char **files, int flagset, struct grep_config *c) {
    regmatch_t match;

    for (reg_node_t *tmp = root->next; tmp != NULL; tmp = tmp->next) {
        int offset = 0;
        int regflags = 0;

        if (IS_SET(flagset, CASE_INS_FL))
            regflags |= CASE_INS_FL;

        int one_flag = 1;
        int len = (int)strlen(c->line);

        if (len == 1 && c->line[0] == '\n') {
            c->line[0] = '\0';
        } else if (len > 1 && c->line[len - 1] == '\n') {
            c->line[len - 1] = '\0';
        }

        while (regexec(tmp->exp, c->line + offset, 1, &match, regflags) == 0) {
            if (one_flag) {
                c->total_matches++;

                if (c->files_n > 1 && !IS_SET(flagset, SUPPRESS_MUL_FNAME_FL) &&
                    !IS_SET(flagset, MATCH_COUNT_FL)) {
                    fprintf(stdout, "%s:", files[c->cur_file]);
                }

                if (IS_SET(flagset, LINE_N_FL) && !IS_SET(flagset, MATCH_COUNT_FL))
                    fprintf(stdout, "%d:", c->cur_line);

                one_flag = 0;
            }

            regflags = REG_NOTBOL;

            if (IS_SET(flagset, CASE_INS_FL))
                regflags |= CASE_INS_FL;
            print_o_string(match, offset, c);
            offset += match.rm_eo;
            if (match.rm_so == match.rm_eo) offset += 1;
            if (offset > len) break;
        }
    }
}

void print_o_string(regmatch_t match, int offset, struct grep_config *c) {
    for (int i = match.rm_so + offset; i < match.rm_eo + offset; i++) {
        if (c->line[i] != '\n')
            putchar(c->line[i]);
    }

    fprintf(stdout, "\n");
}

int match_regex_ll(reg_node_t *root, char *line, int flagset, regmatch_t *offset) {
    int match = 1, regflags = 0;

    if (IS_SET(flagset, CASE_INS_FL))
        regflags |= REG_ICASE;

    for (reg_node_t *tmp = root->next; tmp != NULL; tmp = tmp->next) {
        match = regexec(tmp->exp, line, 2, offset, regflags);
        if (match == 0) {
            break;
        }
    }

    return IS_SET(flagset, INVERSE_FL) ? !match : match;
}

reg_node_t *create_list_of_regexes(char **pfiles, char **inline_patterns, int flagset, int *fail) {
    /* (0) LL initilaization with dummy node */
    reg_node_t *root = init_regex_list("root", NULL);
    reg_node_t *tail = root;

    int rflags = 0;

    if (IS_SET(flagset, CASE_INS_FL)) {
        rflags |= REG_ICASE;
    }

    /* (1) Parse inline busy_ind to the list */
    add_inline_patterns_to_ll(&tail, inline_patterns, rflags);

    /* (2) Parse pfiles busy_ind to the list */
    add_file_patterns_to_ll(&tail, pfiles, rflags, fail);

    return root;
}

void add_inline_patterns_to_ll(reg_node_t **tail, char **inline_patterns, int rflags) {
    for (int i = 0; inline_patterns[i]; i++) {
        regex_t *expr = malloc(sizeof(regex_t));

        int comp_fl = regcomp(expr, inline_patterns[i], rflags);

        if (check_regex_code(comp_fl, expr) == 0) {
            *tail = add_regex_list("stdin", expr, *tail);
        } else {
            free(expr);
        }
    }
}

void add_file_patterns_to_ll(reg_node_t **tail, char **pfiles, int rflags, int *fail) {
    for (int i = 0; pfiles[i]; i++) {
        FILE *fd = fopen(pfiles[i], "r");
        if (!fd) {
            perror("");
            *fail = 1;
            break;
        } else {
            char *line = NULL;
            size_t linecap = 0;
            ssize_t linelen;
            int comp_fl;

            while ((linelen = getline(&line, &linecap, fd)) > 0) {
                regex_t *expr = malloc(sizeof(regex_t));

                if (strcmp(line, "\n") == 0) {
                    /* Empty line pattern in pfile means 'match enything' for grep */
                    comp_fl = regcomp(expr, ".", rflags);
                } else {
                    if (line[strlen(line) - 1] == '\n') {
                        line[strlen(line) - 1] = '\0';
                    }

                    comp_fl = regcomp(expr, line, rflags);
                }

                if (!check_regex_code(comp_fl, expr)) {
                    *tail = add_regex_list(pfiles[i], expr, *tail);
                } else {
                    free(expr);
                }
            }

            if (line) {
                free(line);
            }
        }
        fclose(fd);
    }
}

int check_regex_code(int code, regex_t *expr) {
    if (code) {
        char errbuf[ERRBUF_SIZE];
        regerror(code, expr, errbuf, ERRBUF_SIZE);
        fprintf(stderr, "Regex error!\n");
        fprintf(stderr, "%s\n", errbuf);
    }

    return code;
}

int count_files(char **files) {
    int i = 0;

    if (files[i] == NULL)
        i++;

    int res = 0;
    for (; files[i]; i++) {
        res++;
    }
    return res;
}

void handle_no_pattern_input(char **files, int *busy_ind,
                             char **inline_patterns, int argc, const char **argv) {
    /* If no pattern is specifier, the first non-flag arg becomes pattern */

    inline_patterns[0] = files[0];
    files[0] = NULL;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(inline_patterns[0], argv[i])) {
            busy_ind[i] = 1;
        }
    }
}

void find_files(int argc, const char **argv, const int *busy_ind, char **files) {
    for (int i = 1, j = 0; i < argc; i++) {
        if (!strspn(argv[i], "-") && !busy_ind[i]) {
            files[j] = (char *)argv[i];
            j++;
        }
    }
}

void find_pfiles(int argc, const char **argv, char **pfiles, int *busy_ind) {
    for (int i = 0, m = 0; i < argc; i++) {
        const char *arg = argv[i];

        if (strspn(arg, "-") == 1 && strchr(arg, 'f')) {
            int len = (int)strlen(arg);

            for (int j = 0; j < len; j++) {
                if (arg[j] == 'f' && j != len - 1) {
                    pfiles[m] = (char *)(arg + j + 1);
                    busy_ind[i]++;
                }
            }

            if (!busy_ind[i]) {
                pfiles[m] = (char *)argv[i + 1];
                busy_ind[i + 1]++;
            }
            m++;
        }
    }
}

void find_inline_patterns(int argc, const char **argv, int *busy_ind, char **inline_patterns) {
    for (int i = 0, m = 0; i < argc; i++) {
        const char *arg = argv[i];

        if (strspn(arg, "-") == 1 && strchr(arg, 'e')) {
            int len = (int)strlen(arg);

            for (int j = 0; j < len; j++) {
                if (arg[j] == 'e' && j != len - 1) {
                    inline_patterns[m] = (char *)(arg + j + 1);
                    busy_ind[i]++;
                }
            }

            if (!busy_ind[i]) {
                inline_patterns[m] = (char *)argv[i + 1];
                busy_ind[i + 1]++;
            }
            m++;
        }
    }
}

void print_debug(int flagset, char **files, char **pfiles, char **inline_patterns) {
    printf("Flagset: %d \n", flagset);

    printf("busy_ind: \n");

    for (int i = 0; inline_patterns[i]; i++) {
        printf("Inline P [%d]: %s \n", i, inline_patterns[i]);
    }

    int i = 0;
    do {
        printf("File [%d]: %s \n", i, files[i]);
        i++;
    } while (files[i]);

    for (int m = 0; pfiles[m]; m++) {
        printf("PFile [%d]: %s \n", m, pfiles[m]);
    }

    printf("Flags: \n");

    if (IS_SET(flagset, REGEX_FL))
        printf("-e - Regex  \n");
    if (IS_SET(flagset, CASE_INS_FL))
        printf("-i - Case ins  \n");
    if (IS_SET(flagset, INVERSE_FL))
        printf("-v - Inverse \n");
    if (IS_SET(flagset, MATCH_COUNT_FL))
        printf("-c - Match count flag \n");
    if (IS_SET(flagset, FILE_FL))
        printf("-l - Matching file names  \n");
    if (IS_SET(flagset, LINE_N_FL))
        printf("-n - Number lines  \n");
    if (IS_SET(flagset, SUPPRESS_MUL_FNAME_FL))
        printf("-h - Suppress all file names \n");
    if (IS_SET(flagset, SUPPRESS_ERR_FL))
        printf("-s - Suppress errors flag\n");
    if (IS_SET(flagset, FILE_REGEX_FL))
        printf("-f - Read regex from files\n");
    if (IS_SET(flagset, ONLY_MATCHED_PARTS_FL))
        printf("-o - Only matched parts \n");
}

void parse_grep_flags(const char **argv, int *flag_set, int argc) {
    for (int i = 0; i < argc; i++) {
        const char *arg = argv[i];
        if (strspn(arg, "-") == 1) {
            if (strchr(arg, 'e'))
                SET_BIT(*flag_set, REGEX_FL);
            if (strchr(arg, 'i'))
                SET_BIT(*flag_set, CASE_INS_FL);
            if (strchr(arg, 'v'))
                SET_BIT(*flag_set, INVERSE_FL);
            if (strchr(arg, 'c'))
                SET_BIT(*flag_set, MATCH_COUNT_FL);
            if (strchr(arg, 'l'))
                SET_BIT(*flag_set, FILE_FL);
            if (strchr(arg, 'n'))
                SET_BIT(*flag_set, LINE_N_FL);
            if (strchr(arg, 'h'))
                SET_BIT(*flag_set, SUPPRESS_MUL_FNAME_FL);
            if (strchr(arg, 's'))
                SET_BIT(*flag_set, SUPPRESS_ERR_FL);
            if (strchr(arg, 'f'))
                SET_BIT(*flag_set, FILE_REGEX_FL);
            if (strchr(arg, 'o'))
                SET_BIT(*flag_set, ONLY_MATCHED_PARTS_FL);
        }
    }

    resolve_flag_conflicts(flag_set);
}

reg_node_t *init_regex_list(const char *file, regex_t *expr) {
    reg_node_t *new_node = malloc(sizeof(reg_node_t));

    if (new_node) {
        new_node->exp = expr;
        new_node->file = file;
        new_node->next = NULL;
    }

    return new_node;
}

reg_node_t *add_regex_list(const char *file, regex_t *expr, reg_node_t *last) {
    last->next = init_regex_list(file, expr);
    return last->next;
}

void print_regex_list(reg_node_t *root) {
    while (root) {
        printf("FROM FILE: %s\n", root->file);
        root = root->next;
    }
}

void destroy_regex_list(reg_node_t *root) {
    while (root) {
        reg_node_t *tmp = root;
        root = root->next;
        if (tmp->exp) {
            regfree(tmp->exp);
            free(tmp->exp);
        }
        free(tmp);
    }
}
