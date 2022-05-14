// O FGLAG https://www.ibm.com/docs/en/i/7.2?topic=functions-regexec-execute-compiled-regular-expression

TODO: o

DONE: e, f, v, i, h, s, c, l, n, 

# 1

-c - выдает только количество строк, содержащих образец

grep.c:80
grep.h:0

-l - 	Выдает только имена файлов, содержащих сопоставившиеся строки, по одному в строке. Если образец найден в нескольких строках файла, имя файла не повторяется.

-o o, --only-matching
       Print  only  the  matched  (non-empty) parts of a matching line, with each such part on a separate
       output line.

-o

grep.c:grep
grep.c:grep
grep.c:grep
grep.c:grep
grep.c:grep
grep.c:grep

-on 

grep.c:9:grep
grep.c:11:grep
grep.c:22:grep
grep.c:30:grep
grep.c:36:grep
grep.c:52:grep


+ все флаги из мана и их комбинации 



Как работать с флагом о и участками памяти? 
                     regmatch_t offset[2];

                        size_t nmatch = 2;

                        // printf(
                        //     "With the sub-expression, "
                        //     "a matched substring \"%.*s\" is found at position %d to %d.\n",
                        //     offset[0].rm_eo - offset[0].rm_so, &line[offset[0].rm_so],
                        //     offset[0].rm_so, offset[0].rm_eo - 1);

                        // если заданы флаги INVERT + O, то нужно просто отключить О

                        printf("%.*s\n", offset[0].rm_eo - offset[0].rm_so, &line[offset[0].rm_so]);