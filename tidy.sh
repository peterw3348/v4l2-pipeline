find . -name '*.c' -o -name '*.cpp' | xargs -I{} clang-tidy {} --fix -- -I.
