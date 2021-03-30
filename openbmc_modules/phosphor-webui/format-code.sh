#!/bin/bash

# This script reformats source files using the clang-format utility.
#
# Files are changed in-place, so make sure you don't have anything open in an
# editor, and you may want to commit before formatting in case of awryness.
#
# This must be run on a clean repository to succeed

DIR=$(pwd)
cd ${DIR}

set -e

echo "Formatting code under $DIR/"

: ${CLANG_FORMAT:=clang-format}

# Only validate certain areas of the code base for
# formatting due to some imported code in webui

if [ -f ".clang-format" ]; then
    $CLANG_FORMAT -i `git ls-files '*.js'`
    git --no-pager diff --exit-code
fi
