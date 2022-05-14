#!/bin/bash
cd ..
make fclean
make
cd grep

# VERTER FLAGS: -in -cv -iv -lv -ho -nf

echo "========== Verter Tests: =============="

./s21_grep -in -e aboba notes.md > got.txt
grep -in -e aboba notes.md > expected.txt
diff  got.txt expected.txt

./s21_grep -cv -e aboba notes.md > got.txt
grep -cv -e aboba notes.md > expected.txt
diff  got.txt expected.txt

./s21_grep -iv -e aboba notes.md > got.txt
grep -iv -e aboba notes.md > expected.txt
diff  got.txt expected.txt

./s21_grep -lv -e aboba notes.md > got.txt
grep -lv -e aboba notes.md > expected.txt
diff  got.txt expected.txt

./s21_grep -ho -e aboba notes.md > got.txt
grep -ho -e aboba notes.md > expected.txt
diff got.txt expected.txt

./s21_grep -nf res.txt -e aboba notes.md > got.txt
grep -nf res.txt -e aboba notes.md > expected.txt
diff got.txt expected.txt