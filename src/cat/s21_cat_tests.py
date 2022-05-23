#!python3
from itertools import combinations_with_replacement
from random import shuffle
from os import system

my_cat = "./s21_cat"
cat = "cat"

files = [
    'data-samples/aboba.txt',
    'data-samples/sample.txt',
    'data-samples/sample1.txt',
    'data-samples/test.txt',
    'data-samples/test1.txt',
    'data-samples/test2.txt',
    'data-samples/test3.txt'
]

testing_file = 'testing_file'
testing_file_format = ".ntesting"

flags = {
    '-b',
    '-e',
    '-n',
    '-s',
    '-v',
    '-t'}

all_var = list(combinations_with_replacement(flags, len(flags)))

def check_files(a, b):
    with open(a) as file_1:
        with open(b) as file_2:
            a, b = file_1.read(), file_2.read()
            if a != b:

                count = 0
                for i in range(len(a)):
                    if a[i] == '\n':
                        count += 1
                    if len(b) == i:
                        break
                    if a[i] != b[i]:
                        break

                print("===============A===============")
                print(a[max(0, i-50):min(len(a), i+50)])
                print("===============B===============")
                print(b[max(0, i-50):min(len(b), i+50)])
                print("===============================")
                print("Line:", count, " char:", i)
                input("Press any key to continue:\n")


for test in range(len(all_var)):
    cur_flags_ = all_var[test]
    for cur_flags in (cur_flags_, set(cur_flags_)):
        # test with all flags
        shuffle(files)
        print(f"Current test: {test + 1}\t/\t{len(all_var)}")
        for i, func in (('0', my_cat), ('1', cat)):
            m_str = f'{func} {" ".join(cur_flags)} {" ".join(files)} > {testing_file+i+testing_file_format}'
            print("Command:", m_str)
            system(m_str)
        check_files(testing_file+'0'+testing_file_format,
                    testing_file+'1'+testing_file_format)
        print()

system("rm -rf *.ntesting")