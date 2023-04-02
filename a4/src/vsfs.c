/*
 * This code is provided solely for the personal and private use of students
 * taking the CSC369H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Alexey Khrabrov, Karen Reid, Angela Demke Brown
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2022 Angela Demke Brown
 */

/**
 * CSC369 Assignment 4 - vsfs driver implementation.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// Using 2.9.x FUSE API
#define FUSE_USE_VERSION 29
#include <fuse.h>

#include "bitmap.h"
#include "fs_ctx.h"
#include "map.h"
#include "options.h"
#include "util.h"
#include "vsfs.h"

// NOTE: All path arguments are absolute paths within the vsfs file system and
// start with a '/' that corresponds to the vsfs root directory.
//
// For example, if vsfs is mounted at "/tmp/my_userid", the path to a
// file at "/tmp/my_userid/dir/file" (as seen by the OS) will be
// passed to FUSE callbacks as "/dir/file".
//
// Paths to directories (except for the root directory - "/") do not end in a
// trailing '/'. For example, "/tmp/my_userid/dir/" will be passed to
// FUSE callbacks as "/dir".

/**
 * Initialize the file system.
 *
 * Called when the file system is mounted. NOTE: we are not using the FUSE
 * init() callback since it doesn't support returning errors. This function must
 * be called explicitly before fuse_main().
 *
 * @param fs    file system context to initialize.
 * @param opts  command line options.
 * @return      true on success; false on failure.
 */
static bool
vsfs_init(fs_ctx* fs, vsfs_opts* opts)
{
  size_t size;
  void* image;

  // Nothing to initialize if only printing help
  if (opts->help) {
    return true;
  }

  // Map the disk image file into memory
  image = map_file(opts->img_path, VSFS_BLOCK_SIZE, &size);
  if (image == NULL) {
    return false;
  }

  return fs_ctx_init(fs, image, size);
}

/**
 * Cleanup the file system.
 *
 * Called when the file system is unmounted. Must cleanup all the resources
 * created in vsfs_init().
 */
static void
vsfs_destroy(void* ctx)
{
  fs_ctx* fs = (fs_ctx*)ctx;
  if (fs->image) {
    munmap(fs->image, fs->size);
    fs_ctx_destroy(fs);
  }
}

/** Get file system context. */
static fs_ctx*
get_fs(void)
{
  return (fs_ctx*)fuse_get_context()->private_data;
}

/* Returns the inode number for the element at the end of the path
 * if it exists.  If there is any error, return -1.
 * Possible errors include:
 *   - The path is not an absolute path
 *   - An element on the path cannot be found
 */
static int
path_lookup(const char* path, vsfs_ino_t* ino)
{
  if (path[0] != '/') {
    fprintf(stderr, "Not an absolute path\n");
    return -ENOSYS;
  }

  // TODO: complete this function and any helper functions
  if (strcmp(path, "/") == 0) {
    *ino = VSFS_ROOT_INO;
    return 0;
  }

  return -ENOSYS;
}

/**
 * Get file system statistics.
 *
 * Implements the statvfs() system call. See "man 2 statvfs" for details.
 * The f_bfree and f_bavail fields should be set to the same value.
 * The f_ffree and f_favail fields should be set to the same value.
 * The following fields can be ignored: f_fsid, f_flag.
 * All remaining fields are required.
 *
 * Errors: none
 *
 * @param path  path to any file in the file system. Can be ignored.
 * @param st    pointer to the struct statvfs that receives the result.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_statfs(const char* path, struct statvfs* st)
{
  (void)path; // unused
  fs_ctx* fs = get_fs();
  vsfs_superblock* sb = fs->sb; /* Get ptr to superblock from context */

  memset(st, 0, sizeof(*st));
  st->f_bsize = VSFS_BLOCK_SIZE;  /* Filesystem block size */
  st->f_frsize = VSFS_BLOCK_SIZE; /* Fragment size */
  // The rest of required fields are filled based on the information
  // stored in the superblock.
  st->f_blocks = sb->num_blocks;  /* Size of fs in f_frsize units */
  st->f_bfree = sb->free_blocks;  /* Number of free blocks */
  st->f_bavail = sb->free_blocks; /* Free blocks for unpriv users */
  st->f_files = sb->num_inodes;   /* Number of inodes */
  st->f_ffree = sb->free_inodes;  /* Number of free inodes */
  st->f_favail = sb->free_inodes; /* Free inodes for unpriv users */

  st->f_namemax = VSFS_NAME_MAX; /* Maximum filename length */

  return 0;
}

/**
 * Get file or directory attributes.
 *
 * Implements the lstat() system call. See "man 2 lstat" for details.
 * The following fields can be ignored: st_dev, st_ino, st_uid, st_gid, st_rdev,
 *                                      st_blksize, st_atim, st_ctim.
 * All remaining fields are required.
 *
 * NOTE: the st_blocks field is measured in 512-byte units (disk sectors);
 *       it should include any metadata blocks that are allocated to the
 *       inode (for vsfs, that is the indirect block).
 *
 * NOTE2: the st_mode field must be set correctly for files and directories.
 *
 * Errors:
 *   ENAMETOOLONG  the path or one of its components is too long.
 *   ENOENT        a component of the path does not exist.
 *   ENOTDIR       a component of the path prefix is not a directory.
 *
 * @param path  path to a file or directory.
 * @param st    pointer to the struct stat that receives the result.
 * @return      0 on success; -errno on error;
 */
static int
vsfs_getattr(const char* path, struct stat* st)
{
  if (strlen(path) >= VSFS_PATH_MAX)
    return -ENAMETOOLONG;
  fs_ctx* fs = get_fs();

  memset(st, 0, sizeof(*st));

  // NOTE: This is just a placeholder that allows the file system to be
  //      mounted without errors.
  //      You should remove this from your implementation.
  if (strcmp(path, "/") == 0) {
    // NOTE: all the fields set below are required and must be set
    // according to the information stored in the corresponding inode
    st->st_mode = S_IFDIR | 0777;
    st->st_nlink = 2;
    st->st_size = 0;
    st->st_blocks = 0 * VSFS_BLOCK_SIZE / 512;
    st->st_mtim = (struct timespec){ 0 };
    return 0;
  }

  // TODO: lookup the inode for given path and, if it exists, fill in the
  // required fields based on the information stored in the inode
  (void)fs;
  (void)path_lookup;
  return -ENOSYS;
}

/**
 * Read a directory.
 *
 * Implements the readdir() system call. Should call filler(buf, name, NULL, 0)
 * for each directory entry. See fuse.h in libfuse source code for details.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a filler() call failed).
 *
 * @param path    path to the directory.
 * @param buf     buffer that receives the result.
 * @param filler  function that needs to be called for each directory entry.
 *                Pass 0 as offset (4th argument). 3rd argument can be NULL.
 * @param offset  unused.
 * @param fi      unused.
 * @return        0 on success; -errno on error.
 */
static int
vsfs_readdir(const char* path,
             void* buf,
             fuse_fill_dir_t filler,
             off_t offset,
             struct fuse_file_info* fi)
{
  (void)offset; // unused
  (void)fi;     // unused
  fs_ctx* fs = get_fs();

  // NOTE: This is just a placeholder that allows the file system to be mounted
  // without errors. You should remove this from your implementation.
  if (strcmp(path, "/") == 0) {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    return 0;
  }

  // TODO: lookup the directory inode for given path and iterate through its
  // directory entries
  (void)fs;
  return -ENOSYS;
}

/**
 * Create a directory.
 *
 * Implements the mkdir() system call.
 *
 * You do NOT need to implement this function.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the directory to create.
 * @param mode  file mode bits.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_mkdir(const char* path, mode_t mode)
{
  mode = mode | S_IFDIR;
  fs_ctx* fs = get_fs();

  // OMIT: create a directory at given path with given mode
  (void)path;
  (void)mode;
  (void)fs;
  return -ENOSYS;
}

/**
 * Remove a directory.
 *
 * Implements the rmdir() system call.
 *
 * You do NOT need to implement this function.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a directory.
 *
 * Errors:
 *   ENOTEMPTY  the directory is not empty.
 *
 * @param path  path to the directory to remove.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_rmdir(const char* path)
{
  fs_ctx* fs = get_fs();

  // OMIT: remove the directory at given path (only if it's empty)
  (void)path;
  (void)fs;
  return -ENOSYS;
}

/**
 * Create a file.
 *
 * Implements the open()/creat() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" doesn't exist.
 *   The parent directory of "path" exists and is a directory.
 *   "path" and its components are not too long.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *
 * @param path  path to the file to create.
 * @param mode  file mode bits.
 * @param fi    unused.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
  (void)fi; // unused
  assert(S_ISREG(mode));
  fs_ctx* fs = get_fs();

  // TODO: create a file at given path with given mode
  (void)path;
  (void)mode;
  (void)fs;
  return -ENOSYS;
}

/**
 * Remove a file.
 *
 * Implements the unlink() system call.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors: none
 *
 * @param path  path to the file to remove.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_unlink(const char* path)
{
  fs_ctx* fs = get_fs();

  // TODO: remove the file at given path
  (void)path;
  (void)fs;
  return -ENOSYS;
}

/**
 * Change the modification time of a file or directory.
 *
 * Implements the utimensat() system call. See "man 2 utimensat" for details.
 *
 * NOTE: You only need to implement the setting of modification time (mtime).
 *       Timestamp modifications are not recursive.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists.
 *
 * Errors: none
 *
 * @param path   path to the file or directory.
 * @param times  timestamps array. See "man 2 utimensat" for details.
 * @return       0 on success; -errno on failure.
 */
static int
vsfs_utimens(const char* path, const struct timespec times[2])
{
  fs_ctx* fs = get_fs();
  vsfs_inode* ino = NULL;

  // TODO: update the modification timestamp (mtime) in the inode for given
  // path with either the time passed as argument or the current time,
  // according to the utimensat man page
  (void)path;
  (void)fs;
  (void)ino;

  // 0. Check if there is actually anything to be done.
  if (times[1].tv_nsec == UTIME_OMIT) {
    // Nothing to do.
    return 0;
  }

  // 1. TODO: Find the inode for the final component in path

  // 2. Update the mtime for that inode.
  //    This code is commented out to avoid failure until you have set
  //    'ino' to point to the inode structure for the inode to update.
  if (times[1].tv_nsec == UTIME_NOW) {
    // if (clock_gettime(CLOCK_REALTIME, &(ino->i_mtime)) != 0) {
    // clock_gettime should not fail, unless you give it a
    // bad pointer to a timespec.
    //	assert(false);
    //}
  } else {
    // ino->i_mtime = times[1];
  }

  // return 0;
  return -ENOSYS;
}

/**
 * Change the size of a file.
 *
 * Implements the truncate() system call. Supports both extending and shrinking.
 * If the file is extended, the new uninitialized range at the end must be
 * filled with zeros.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *   EFBIG   write would exceed the maximum file size.
 *
 * @param path  path to the file to set the size.
 * @param size  new file size in bytes.
 * @return      0 on success; -errno on error.
 */
static int
vsfs_truncate(const char* path, off_t size)
{
  fs_ctx* fs = get_fs();

  // TODO: set new file size, possibly "zeroing out" the uninitialized range
  (void)path;
  (void)size;
  (void)fs;
  return -ENOSYS;
}

/**
 * Read data from a file.
 *
 * Implements the pread() system call. Must return exactly the number of bytes
 * requested except on EOF (end of file). Reads from file ranges that have not
 * been written to must return ranges filled with zeros. You can assume that the
 * byte range from offset to offset + size is contained within a single block.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors: none
 *
 * @param path    path to the file to read from.
 * @param buf     pointer to the buffer that receives the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to read from.
 * @param fi      unused.
 * @return        number of bytes read on success; 0 if offset is beyond EOF;
 *                -errno on error.
 */
static int
vsfs_read(const char* path,
          char* buf,
          size_t size,
          off_t offset,
          struct fuse_file_info* fi)
{
  (void)fi; // unused
  fs_ctx* fs = get_fs();

  // TODO: read data from the file at given offset into the buffer
  (void)path;
  (void)buf;
  (void)size;
  (void)offset;
  (void)fs;
  return -ENOSYS;
}

/**
 * Write data to a file.
 *
 * Implements the pwrite() system call. Must return exactly the number of bytes
 * requested except on error. If the offset is beyond EOF (end of file), the
 * file must be extended. If the write creates a "hole" of uninitialized data,
 * the new uninitialized range must filled with zeros. You can assume that the
 * byte range from offset to offset + size is contained within a single block.
 *
 * Assumptions (already verified by FUSE using getattr() calls):
 *   "path" exists and is a file.
 *
 * Errors:
 *   ENOMEM  not enough memory (e.g. a malloc() call failed).
 *   ENOSPC  not enough free space in the file system.
 *   EFBIG   write would exceed the maximum file size
 *
 * @param path    path to the file to write to.
 * @param buf     pointer to the buffer containing the data.
 * @param size    buffer size (number of bytes requested).
 * @param offset  offset from the beginning of the file to write to.
 * @param fi      unused.
 * @return        number of bytes written on success; -errno on error.
 */
static int
vsfs_write(const char* path,
           const char* buf,
           size_t size,
           off_t offset,
           struct fuse_file_info* fi)
{
  (void)fi; // unused
  fs_ctx* fs = get_fs();

  // TODO: write data from the buffer into the file at given offset, possibly
  // "zeroing out" the uninitialized range
  (void)path;
  (void)buf;
  (void)size;
  (void)offset;
  (void)fs;
  return -ENOSYS;
}

static struct fuse_operations vsfs_ops = {
  .destroy = vsfs_destroy,
  .statfs = vsfs_statfs,
  .getattr = vsfs_getattr,
  .readdir = vsfs_readdir,
  .mkdir = vsfs_mkdir,
  .rmdir = vsfs_rmdir,
  .create = vsfs_create,
  .unlink = vsfs_unlink,
  .utimens = vsfs_utimens,
  .truncate = vsfs_truncate,
  .read = vsfs_read,
  .write = vsfs_write,
};

int
main(int argc, char* argv[])
{
  vsfs_opts opts = { 0 }; // defaults are all 0
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  if (!vsfs_opt_parse(&args, &opts))
    return 1;

  fs_ctx fs = { 0 };
  if (!vsfs_init(&fs, &opts)) {
    fprintf(stderr, "Failed to mount the file system\n");
    return 1;
  }

  return fuse_main(args.argc, args.argv, &vsfs_ops, &fs);
}
