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

if [ $# -lt 1 ]; then
  echo 1>&2 "$0: not enough arguments"
  exit 2
elif [ $# -gt 1 ]; then
  echo 1>&2 "$0: too many arguments"
  exit 2
fi

BUILD_DIR="$1"
if [ ! -d "$BUILD_DIR" ]
then
    echo 1>&2 "$0: The directory $BUILD_DIR does not exist"
    exit 2
fi

# import common functions
source "${BASH_SOURCE%/*}"/util.sh

OUT_DIR="$BUILD_DIR/test_mkfs"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

# Echo every command made from here on out. This is very verbose, but potentially useful for debugging.
set -x
exec 2>"$OUT_DIR/test_mkfs.log"

test_mkfs "mkfs_small" 1048576 64           # 1 MB
test_mkfs "mkfs_medium" 16777216 128        # 16 MB
# Consider adding more calls to test_mkfs with different sizes and inode counts
