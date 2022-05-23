#ifndef SRC_COMMON_S21_COMMON_H_
#define SRC_COMMON_S21_COMMON_H_

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#define IS_SET(A, N) ((A) & (1 << (N)))
#define SET_BIT(A, N) ((A) |= (1 << (N)))
#define CLEAR_BIT(A, N) ((A) &= ~(1 << (N)))
#define FLIP_BIT(A, N) ((A) ^= (1 << (N)))

#define ERRBUF_SIZE 512

#endif  // SRC_COMMON_S21_COMMON_H_
