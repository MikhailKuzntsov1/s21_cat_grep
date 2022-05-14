#include "s21_cat.h"

int main(int argc, char **argv) {
    s21_cat(argc, argv);
    return EXIT_SUCCESS;
}

void s21_cat(int argc, char **argv) {
    if (argc >= 2) {
        char **files = malloc(sizeof(char *) * argc);

        if (files) {
            int files_len = 0;
            int flag_set = parse_flag_set(argc, argv, files, &files_len);

            if (flag_set != INCORRECT_SET) {
                if ((IS_SET(flag_set, NUMBER_NON_BLANK) && IS_SET(flag_set, NUMBER_LINES)))
                    CLEAR_BIT(flag_set, NUMBER_LINES);

                for (int i = 0; i < files_len; i++) {
                    FILE *file = fopen(files[i], "r");

                    if (errno)
                        perror("Error: ");
                    if (file) {
                        process_file(file, flag_set);
                        process_file(file, RESET_FL);
                        fclose(file);
                    }
                }
            } else {
                printf("Incorrect flags!\n");
            }

            free(files);
        }
    }
}

int parse_flag_set(int argc, char **argv, char **files, int *files_len) {
    int flag_set = 0;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        int test1 = parse_single_flags(arg, &flag_set);
        int test2 = parse_double_flags(arg, &flag_set);

        if (test1 == -1 || test2 == -1) {
            flag_set = INCORRECT_SET;
            break;
        }

        if (!test1 && !test2) {
            files[*files_len] = arg;
            (*files_len)++;
        }
    }

    return flag_set;
}

int parse_single_flags(char *arg, int *flag_set) {
    int res = 0;
    if (strspn(arg, "-") == 1) {
        res = 1;
        if (strchr(arg, 'e') || strchr(arg, 'E'))
            SET_BIT(*flag_set, EOL_DOLLAR);
        else if (strchr(arg, 'n'))
            SET_BIT(*flag_set, NUMBER_LINES);
        else if (strchr(arg, 'b'))
            SET_BIT(*flag_set, NUMBER_NON_BLANK);
        else if (strchr(arg, 's'))
            SET_BIT(*flag_set, SQUEEZE_ADJ);
        else if (strchr(arg, 't') || strchr(arg, 'T'))
            SET_BIT(*flag_set, TABS);
        else if (strchr(arg, 'e') || strchr(arg, 't') || strchr(arg, 'v'))
            SET_BIT(*flag_set, VERBOSE);
        else
            res = -1;
    }

    return res;
}

int parse_double_flags(char *arg, int *flag_set) {
    int res = 0;
    if (strspn(arg, "--") == 2) {
        res = 1;
        if (strstr(arg, "number-nonblank"))
            SET_BIT(*flag_set, NUMBER_NON_BLANK);
        else if (strstr(arg, "number"))
            SET_BIT(*flag_set, NUMBER_LINES);
        else if (strstr(arg, "squeeze-blank"))
            SET_BIT(*flag_set, SQUEEZE_ADJ);
        else
            res = -1;
    }

    return res;
}

void process_file(FILE *file, int flags) {
    static struct cat_state st = {
        .buffer = {'\0'}, .line = 1, .nl = 1, .nb = 0, .prev_prev = 0, .prev = 0};

    if (flags == RESET_FL)
        reset_state(&st);

    while (feof(file) != EOF && file && flags != RESET_FL) {
        int ch = getc(file);

        if (ch == EOF)
            break;

        if (apply_squeeze_flag(ch, &st, flags))
            continue;

        st.prev_prev = st.prev;
        st.prev = ch;

        apply_eol_flag(ch, &st, flags);
        apply_number_lines_flag(&st, flags);

        st.buffer[0] = ch;

        handle_nl_char(ch, &st, flags);
        apply_number_non_blank_flag(&st, flags);
        apply_spec_symb_flag(ch, &st, flags);
        apply_tabs_flag(ch, &st, flags);

        fprintf(stdout, "%s", st.buffer);
        memset(st.buffer, '\0', MAX_CHAR_LEN);
    }

    if (errno)
        perror("Error: ");
}

void reset_state(struct cat_state *st) {
#if defined(__MACH__) || defined(__APPLE__)
    st->line = 1;
#endif
    st->nl = 1;
    st->nb = 0;
    st->prev_prev = 0;
    st->prev = 0;
    st->iter = 0;
}

int apply_squeeze_flag(char cur_ch, struct cat_state *cur_config, int flags) {
    return (IS_SET(flags, SQUEEZE_ADJ) && cur_config->prev_prev == '\n' &&
            cur_config->prev == '\n' && cur_ch == '\n');
}

void apply_spec_symb_flag(int ch, struct cat_state *st, int flags) {
    if (IS_SET(flags, VERBOSE) && ((ch >= 0 && ch <= 32) || (ch >= 127 && ch <= 255)) && ch != '\n') {
        for (size_t i = 0; i < strlen(s21_cat_spec_symbols[ch]); i++) {
            st->buffer[i] = s21_cat_spec_symbols[ch][i];
        }
    }
}

void apply_tabs_flag(char ch, struct cat_state *st, int flags) {
    if (IS_SET(flags, TABS) && ch == '\t') {
        st->buffer[0] = '^';
        st->buffer[1] = 'I';
    }
}

void apply_eol_flag(char ch, struct cat_state *st, int flags) {
    if (IS_SET(flags, EOL_DOLLAR) && ch == '\0') {
        st->buffer[0] = '$';
        st->buffer[1] = '\n';
    }
}

void apply_number_lines_flag(struct cat_state *st, int flags) {
    if (IS_SET(flags, NUMBER_LINES) && st->nl) {
        fprintf(stdout, "%6d\t", st->line);
        st->line++;
        st->nl = 0;
    }
}

void handle_nl_char(char ch, struct cat_state *st, int flags) {
    if (ch == '\n') {
        st->nl = 1, st->nb = 0;

        if (IS_SET(flags, EOL_DOLLAR)) {
            st->buffer[0] = '$';
            st->buffer[1] = '\n';
        }
    } else {
        st->nb = 1;
    }
}

void apply_number_non_blank_flag(struct cat_state *st, int flags) {
    if (IS_SET(flags, NUMBER_NON_BLANK) && st->nb && st->nl) {
        fprintf(stdout, "%6d\t", st->line);
        st->line++;
        st->nl = 0;
    }
}
