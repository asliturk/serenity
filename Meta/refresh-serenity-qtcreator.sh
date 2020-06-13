#!/bin/sh

export SERENITY_ROOT=`git rev-parse --show-toplevel`

cd "$SERENITY_ROOT" || exit 1
find . -name '*.ipc' -or -name '*.cpp' -or -name '*.c' -or -name '*.h' -or -name '*.S' -or -name '*.css'  -or -name 'CMakeLists.txt' | grep -Fv Patches/ | grep -Fv Root/ | grep -Fv Ports/ | grep -Fv Toolchain/ | grep -Fv Base/ > serenity.files
