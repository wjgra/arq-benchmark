#!/usr/bin/env bash

# Apply automatic formatting to all hpp/cpp files in the lib
for file in $(find arq-benchmark \( -name "*.hpp" -o  -name "*.cpp" \)); do
    clang-format -style=file -i "${file}"
done