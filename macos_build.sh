#!/usr/bin/env bash

# -time             Time individual commands.
# -ftime-report     Print timing summary of each stage of compilation.


IGNORED_WARNINGS="-Wno-gnu-anonymous-struct -Wno-return-type -Wno-deprecated-declarations"
COMPILER_FLAGS_SLOW="-Og -g -Werror -pedantic -pedantic-errors -pthread"
COMPILER_FLAGS_FAST="-O2    -Werror -pedantic -pedantic-errors -pthread"
LINKER_FLAGS=""

mkdir -p build

pushd build > /dev/null

if [ "$1" = "fast" ]; then
    echo "Building fast"
    clang++ ${COMPILER_FLAGS_FAST}  ${IGNORED_WARNINGS} ../macos_platform.cpp ${LINKER_FLAGS} -o main > /dev/null
else
    echo "Building slow"
    clang++ ${COMPILER_FLAGS_SLOW}  ${IGNORED_WARNINGS} ../macos_platform.cpp ${LINKER_FLAGS} -o main > /dev/null
fi


popd
