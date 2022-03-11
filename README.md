# refactoring_tool:

## Refactoring tool for source to source transformation of floating point applications to posit

It has been tested with clang version 10.
I have followed the tutorial from [Eli Bendersky's website](https://eli.thegreenplace.net/2014/07/29/ast-matchers-and-clang-refactoring-tools)

1. To build set path for llvm and clang in Makefile and run make in top directory.
2. To run:
    ./build/refactor_posit examples/ex10.c --
 
It will save ex10_pos in same directory.

