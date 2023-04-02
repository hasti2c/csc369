# This code is provided solely for the personal and private use of students
# taking the CSC369H course at the University of Toronto. Copying for purposes
# other than this use is expressly prohibited. All forms of distribution of
# this code, including but not limited to public repositories on GitHub,
# GitLab, Bitbucket, or any other online platform, whether as given or with
# any changes, are expressly prohibited.
#
# Authors: Mario Badr, Alexey Khrabrov, Karen Reid, Angela Demke Brown
#
# All of the files in this directory and all subdirectories are:
# Copyright (c) 2022 Angela Demke Brown

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse)

function(target_link_fuse exe_name)
  target_link_libraries(
      ${exe_name}
      PRIVATE
        ${FUSE_LDFLAGS}
  )

  target_compile_options(
      ${exe_name}
      PRIVATE
        ${FUSE_CFLAGS}
        -Wall -Wextra -Werror
  )
endfunction()

add_executable(vsfs
    vsfs.c
    vsfs.h
    fs_ctx.c
    fs_ctx.h
    options.h
    options.c
    bitmap.h
    bitmap.c
    map.h
    map.c
)
target_link_fuse(vsfs)

add_executable(mkfs.vsfs
    mkfs.c
    bitmap.h
    bitmap.c
    map.h
    map.c
)

target_compile_options(
  mkfs.vsfs
  PRIVATE
    -Wall -Wextra -Werror
)
