cp ../materials/linters/CPPLINT.cfg .

find . -type f -name "*.c" | xargs python3 ../materials/linters/cpplint.py --extensions=c
find . -type f -name "*.h" | xargs python3 ../materials/linters/cpplint.py --extensions=c

#find . -type f -name "*.c" | xargs ~/homebrew/bin/cppcheck
#find . -type f -name "*.c" | xargs ~/homebrew/bin/cppcheck --enable=all --suppress=missingIncludeSystem
#find . -type f -name "*.c" | xargs cppcheck
find . -type f -name "*.c" | xargs cppcheck --enable=all --suppress=missingIncludeSystem

# add leaks & vg check to this script

rm -f CPPLINT.cfg