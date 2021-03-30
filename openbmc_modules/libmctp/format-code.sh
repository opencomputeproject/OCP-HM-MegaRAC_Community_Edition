#!/bin/bash

: ${CLANG_FORMAT:=clang-format}

FILES=$(comm -3 <(sort .clang-ignore) <(git ls-files | grep '.*\.[ch]$' | sort))

if [ -n "$FILES" ]
then
    $CLANG_FORMAT -style=file -i $FILES
fi
