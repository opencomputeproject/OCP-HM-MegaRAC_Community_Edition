#!/bin/bash

# This script reformats source files using the clang-format utility.
#
# Files are changed in-place, so make sure you don't have anything open in an
# editor, and you may want to commit before formatting in case of awryness.
#
# This must be run on a clean repository to succeed
#
# Input parmameter must be full path to git repo to scan

DIR=$1
cd ${DIR}

set -e

echo "Formatting code under $DIR/"

if [[ -f "setup.cfg" ]]; then
  pycodestyle --show-source .
  rc=$?
  if [[ ${rc} -ne 0 ]]; then
    exit ${rc}
  fi
fi

# If .shellcheck exists, stop on error.  Otherwise, allow pass.
if [[ -f ".shellcheck" ]]; then
  shellcheck_allowfail="false"
else
  shellcheck_allowfail="true"
fi

# Run shellcheck on any shell-script.
shell_scripts="$(git ls-files | xargs -n1 file -0 | \
                 grep -a "shell script" | cut -d '' -f 1)"
for script in ${shell_scripts}; do
  shellcheck -x "${script}" || ${shellcheck_allowfail}
done

# Allow called scripts to know which clang format we are using
export CLANG_FORMAT="clang-format-10"
IGNORE_FILE=".clang-ignore"
declare -a IGNORE_LIST

if [[ -f "${IGNORE_FILE}" ]]; then
  readarray -t IGNORE_LIST < "${IGNORE_FILE}"
fi

ignorepaths=""
ignorefiles=""

for path in "${IGNORE_LIST[@]}"; do
  # Check for comment, line starting with space, or zero-length string.
  # Checking for [[:space:]] checks all options.
  if [[ -z "${path}" ]] || [[ "${path}" =~ ^(#|[[:space:]]).*$ ]]; then
    continue
  fi

  # All paths must start with ./ for find's path prune expectation.
  if [[ "${path}" =~ ^\.\/.+$ ]]; then
    ignorepaths+=" ${path}"
  else
    ignorefiles+=" ${path}"
  fi
done

searchfiles=""
while read path; do
  # skip ignorefiles
  if [[ $ignorefiles == *"$(basename "${path}")"* ]]; then
    continue
  fi

  skip=false
  #skip paths in ingorepaths
  for pathname in $ignorepaths; do
    if [[ "./${path}" == "${pathname}"* ]]; then
       skip=true
       break
    fi
  done

  if [ "$skip" = true ]; then
   continue
  fi
  searchfiles+="\"./${path}\" "

# Get C and C++ files managed by git and skip the mako files
done <<<$(git ls-files | grep -e '\.[ch]pp$' -e '\.[ch]$' | grep -v '\.mako\.')

if [[ -f ".clang-format" ]]; then
  echo ${searchfiles} | xargs "${CLANG_FORMAT}" -i
  git --no-pager diff --exit-code
fi

# Sometimes your situation is terrible enough that you need the flexibility.
# For example, phosphor-mboxd.
if [[ -f "format-code.sh" ]]; then
  ./format-code.sh
  git --no-pager diff --exit-code
fi
