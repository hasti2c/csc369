#!/bin/bash

benchmark_name=$(basename "$1")
traces_dir="../traces"

set -e -u -o pipefail

# Run valgrind to produce the trace
valgrind --tool=lackey --trace-mem=yes "${1}" ${@:2} >& "tmp"

# Trim the trace so that it only includes the algorithm part of the program
mkdir -p "${traces_dir}"
./trimtrace.py "${benchmark_name}.marker" "tmp" > "${traces_dir}/addr-${benchmark_name}.ref"
rm -f "tmp"

# Reduce the trace size of page level accesses keeping the page offset. Then
# transform the trace to reduce the range of the page offset and generate
# values for the store/modify and load/ifetch operations.
./fastslim-with-offset.py --keepcode --buffersize 8 < "${traces_dir}/addr-${benchmark_name}.ref" | ./simify-trace.py --simpagesize 16 > "${traces_dir}/simvaddr-${benchmark_name}.ref"

rm "${benchmark_name}.marker"
