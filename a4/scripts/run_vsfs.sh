#!/bin/bash

#  This code is provided solely for the personal and private use of students
#  taking the CSC369H course at the University of Toronto. Copying for purposes
#  other than this use is expressly prohibited. All forms of distribution of
#  this code, including but not limited to public repositories on GitHub,
#  GitLab, Bitbucket, or any other online platform, whether as given or with
#  any changes, are expressly prohibited.
# 
#  Authors: Mario Badr
# 
#  All of the files in this directory and all subdirectories are:
#  Copyright (c) 2023 Mario Badr


if [ $# -lt 2 ]; then
  echo 1>&2 "$0: not enough arguments"
  exit 2
elif [ $# -gt 2 ]; then
  echo 1>&2 "$0: too many arguments"
  exit 2
fi

BUILD_DIR="$1"
if [ ! -d "$BUILD_DIR" ]
then
    echo 1>&2 "$0: The directory $BUILD_DIR does not exist"
    exit 2
fi

TESTS_DIR="$2"
if [ ! -d "$TESTS_DIR" ]
then
    echo 1>&2 "$0: The directory $TESTS_DIR does not exist"
    exit 2
fi

# import common functions
source "${BASH_SOURCE%/*}"/util.sh

OUT_DIR="$BUILD_DIR/test_vsfs"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

# Echo every command made from here on out. This is very verbose, but potentially useful for debugging.
set -x
exec 2>"$OUT_DIR/test_vsfs.log"

# teach.cs specific. if you are locally, comment this out and assign the variable to a local dir
CSC369_A4_DISK_DIR="/u/csc369h/winter/pub/a4"
LOCAL_A4_DISK_DIR="${BASH_SOURCE%/*}"/../images
mkdir -p "$LOCAL_A4_DISK_DIR"

test_vsfs "vsfs-empty" "$CSC369_A4_DISK_DIR/vsfs-empty.disk" 64 "$TESTS_DIR"
test_vsfs "vsfs-1file" "$CSC369_A4_DISK_DIR/vsfs-1file.disk" 64 "$TESTS_DIR"
test_vsfs "vsfs-3file" "$CSC369_A4_DISK_DIR/vsfs-3file.disk" 128 "$TESTS_DIR"
test_vsfs "vsfs-many" "$CSC369_A4_DISK_DIR/vsfs-many.disk" 256 "$TESTS_DIR"
test_vsfs "vsfs-manysizes" "$CSC369_A4_DISK_DIR/vsfs-manysizes.disk" 192 "$TESTS_DIR"
test_vsfs "vsfs-maxfs" "$CSC369_A4_DISK_DIR/vsfs-maxfs.disk" 512 "$TESTS_DIR"
