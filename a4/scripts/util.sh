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

# Some general test constraints
A4_TIMEOUT=30s
MAX_FEEDBACK_FILE_SIZE=90000000

# Important paths
CSC369_A4_DIR="/u/csc369h/winter/pub/a4/"  # for teach.cs

# The fsck executables can only be found on teach.cs
FSCK_MKFS_EXE="$CSC369_A4_DIR/fsck.mkfs"
FSCK_VSFS_EXE="$CSC369_A4_DIR/fsck.vsfs"

# These executables must be compiled by you
MKFS_EXE="bin/mkfs.vsfs"
VSFS_EXE="bin/vsfs"

# compile_submission(compile_dir, build, compile_log, compile_args)
#   Compile the submission found in compile_dir using CMake and the given build.
#   Arguments to CMake's build (not configure) step are given via compile_args.
#   Redirect stdout and stderr of all operations to compile_log.
compile_submission() {
  {
    echo "Configuring debug build with CMake"
    cmake -H"$1/.." -B"$1" -DCMAKE_BUILD_TYPE="$3"

    echo "Compiling with CMake"
    cmake --build "$1" "${@:4}"
  } >> "$2" 2>&1
}

# run_binary(binary_exe, binary_out, binary_args)
#   Run the binary at location binary_exe with the A4_TIMEOUT timeout.
#   Arguments to binary_exe are given via binary_args.
#   Redirect stdout and stderr of the binary_exe to binary_out.
run_binary() {
  # Create an empty file (if it doesn't exist) before redirecting stdout/stderr to it
  touch "$2"
  # Run the binary, but with a timeout
  timeout "$A4_TIMEOUT" "$1" "${@:3}" >> "$2" 2>&1

  # Protect against very large outputs
  OUT_SIZE=$(wc -c <"$2")
  if [ "$OUT_SIZE" -ge $MAX_FEEDBACK_FILE_SIZE ]; then
    truncate -s "$MAX_FEEDBACK_FILE_SIZE" "$2"
  fi
}

# create_file(file_path, file_size, replace)
#   Create (or replace) a file at location image_path so that it has image_size number of bytes.
create_file() {
  # Force-remove the file when replace is true
  if $3 ; then rm -f "$1" ; fi

  # Create the file with the given size
  truncate -s "$2" "$1"
}

# test_mkfs(name, image_size, inode_count)
#   Test the mkfs.vsfs executable by:
#     1. Creating an a new file of image_size in BUILD_DIR
#     2. Running mkfs.vsfs on the new file with inode_count inodes
#     3. Checking the consistency of the new image using fsck.mkfs (found on teach.cs)
#
#   Precondition: Assumes that the project has been successfully compiled in BUILD_DIR
test_mkfs() {
  image="$OUT_DIR/$1.disk"

  out_file="$OUT_DIR/$1.out"
  fsck_out_file="$OUT_DIR/$1-fsck.out"

  echo "Creating the image file $image"
  create_file "$image" "$2" true

  echo "Making a file system with $3 inodes."
  run_binary "$BUILD_DIR/$MKFS_EXE" "$out_file" -i "$3" "$image"

  echo "Checking consistency."
  run_binary "$FSCK_MKFS_EXE" "$fsck_out_file" -i "$3" "$image" -a

  echo "Done. See $OUT_DIR for all redirected output files."
}

# test_vsfs(name, image, inode_count, tests_dir)
test_vsfs() {
  echo "Starting $1 test"

  mount_point="/tmp/$USER"
  out_file="$OUT_DIR/$1.out"
  test_out_file="$OUT_DIR/$1-pytests.out"
  fsck_out_file="$OUT_DIR/$1-fsck.out"

  mkdir -p "$mount_point"

  # run the binary in the background and without a timeout
  echo "Mounting $2 at $mount_point"
  "$BUILD_DIR/$VSFS_EXE" "$2" "$mount_point" -f -d >> "$out_file" 2>&1 &
  VSFS_PID=$! # save the PID of the last background process (i.e., vsfs)
  sleep 1     # give vsfs a second...
  
  if ps -p "$VSFS_PID" > /dev/null
  then
    # ...vsfs is still running, hopefully without error

    # run all pytests (i.e., test_*.py) in the tests directory
    echo "Running pytest tests in $4."
    python3 -m pytest "$4" -c /dev/null -v --mount_point "$mount_point" --inode_count "$3" --disk "$2" >> "$test_out_file" 2>&1
  
    # Unmounting should cause the VSFS_PID process to terminate
    echo "Unmounting $mount_point"
    fusermount --unmount "$mount_point"
  fi

  echo "Checking consistency."
  run_binary "$FSCK_VSFS_EXE" "$fsck_out_file" -i "$3" "$2" -a

  rmdir "$mount_point"

  echo "Done. See $OUT_DIR for all redirected output files."
}
