#ifndef SRC_GREP_S21_GREP_H_
#define SRC_GREP_S21_GREP_H_

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/s21_common.h"

struct grep_config {
    int cur_file;
    int files_n;
    int total_matches;
    int cur_line;
    int non_empty;
    regmatch_t match[2];
    char *line;
};

enum memory_mode {
    MEMORY_ALLOCATION,
    MEMORY_FREE
};

typedef enum grep_flags {
    REGEX_FL,
    CASE_INS_FL,
    INVERSE_FL,
    MATCH_COUNT_FL,
    FILE_FL,
    LINE_N_FL,
    SUPPRESS_MUL_FNAME_FL,
    SUPPRESS_ERR_FL,
    FILE_REGEX_FL,
    ONLY_MATCHED_PARTS_FL
} grep_flags;

typedef struct reg_node_t {
    regex_t *exp;
    const char *file;
    struct reg_node_t *next;
} reg_node_t;

reg_node_t *init_regex_list(const char *file, regex_t *expr);
reg_node_t *add_regex_list(const char *file, regex_t *expr, reg_node_t *last);
void print_regex_list(reg_node_t *root);
void destroy_regex_list(reg_node_t *root);

/* (0) Helpers */

int check_regex_code(int code, regex_t *expr);
void print_debug(int flagset, char **files, char **pfiles, char **inline_patterns);
int count_files(char **files);
void resolve_flag_conflicts(int *flags);
// free_malloced_buffers(int argc, ...);

/* (1) ARGV parsing stage */
void parse_grep_flags(const char **argv, int *flag_set, int argc);
void find_files(int argc, const char **argv, const int *busy_ind, char **files);
void find_pfiles(int argc, const char **argv, char **pfiles, int *busy_ind);
void find_inline_patterns(int argc, const char **argv, int *busy_ind, char **inline_patterns);
void handle_no_pattern_input(char **files, int *busy_ind,
                             char **inline_patterns, int argc, const char **argv);

/* (2) Regex parsing, compilation, application stage */

reg_node_t *create_list_of_regexes(char **pfiles, char **inline_patterns, int flagset, int *fail);
void add_inline_patterns_to_ll(reg_node_t **tail, char **inline_patterns, int rflags);
void add_file_patterns_to_ll(reg_node_t **tail, char **pfiles, int rflags, int *fail);
void grep(reg_node_t *root, char **files, int flagset);
int match_regex_ll(reg_node_t *root, char *line, int flagset, regmatch_t *offset);

/* (3) Output handling & flag application stage */
void handle_o_output(reg_node_t *root, char **files, int flagset, struct grep_config *c);
void update_config(struct grep_config *c);
void reset_config(struct grep_config *c);
void print_o_string(regmatch_t match, int offset, struct grep_config *c);
int count_matches_for_line(reg_node_t *root, char *line, int flagset, regmatch_t *offset);
void free_existing_memory(char **p1, char **p2, char **p3, int *p4, reg_node_t *p5);
void grep_argv_parser(const char **argv, int argc,
                      int flagset, char **pfiles, int *busy_ind, char **inline_patterns, char **files);

#endif  // SRC_GREP_S21_GREP_H_
