#!/bin/sh

script_path=$(cd -P -- "$(dirname -- "$0")" && pwd -P)
cd "$script_path/" || exit 1

find code/modules/ -type f -print0 | xargs -0 dos2unix -ic0 | xargs -0 dos2unix -b
find code/game/ -type f -print0 | xargs -0 dos2unix -ic0 | xargs -0 dos2unix -b
