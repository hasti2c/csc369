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

/**
 * Find dentry with name in block.
 * Returns 0 if successful, -ENOENT if no such dentry.
 */
static int
lookup_in_block(const char* name, vsfs_ino_t* ino, vsfs_blk_t blk) {
  fs_ctx *fs = get_fs();
  vsfs_dentry* blk_ptr = (vsfs_dentry*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    if (strcmp(blk_ptr[i].name, name) == 0) {
      *ino = blk_ptr[i].ino;
      return 0;
    }
  }
  return -ENOENT;
}

/**
 * Find dentry with name in indirect block.
 * Returns 0 if successful, -ENOENT if no such dentry.
 */
static int
lookup_in_indirect_block(const char* name, vsfs_ino_t* ino, vsfs_blk_t blk) {
  fs_ctx *fs = get_fs();
  vsfs_blk_t* sub_blks = (vsfs_blk_t*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_ino_t); i++) {
    if (sub_blks[i] != 0 && lookup_in_block(name, ino, sub_blks[i]) == 0) 
      return 0;
  }
  return -ENOENT;
}


/* Finds the inode number for the element at the end of the path
 * if it exists.  
 *
 * Errors:
 *   ENOENT   An element on the path cannot be found
 *   ENOTDIR  The path is not an abosulte path
 *
 * @param path  path to a file or directory.
 * @param pointer to the ino that receives the result.
 * @return 0 on success; -errno on error.
 */
static int
path_lookup(const char* path, vsfs_ino_t* ino)
{
  if (path[0] != '/') {
    fprintf(stderr, "Not an absolute path\n");
    return -ENOTDIR;
  }

  if (strcmp(path, "/") == 0) {
    *ino = VSFS_ROOT_INO;
    return 0;
  }
 
  // file (since no subdirectories)
  fs_ctx* fs = get_fs();
  vsfs_inode* root_ino = &fs->itable[VSFS_ROOT_INO];
  for (uint32_t i = 0; i < VSFS_NUM_DIRECT; i++) {
    if (root_ino->i_direct[i] != 0 && lookup_in_block(path + 1, ino, root_ino->i_direct[i]) == 0)
      return 0;
  }
  if (root_ino->i_indirect != 0)
    return lookup_in_indirect_block(path + 1, ino, root_ino->i_indirect);
  return -ENOENT;
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

  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  int err = path_lookup(path, ino_ind);
  if (err < 0)
    return err;
  vsfs_inode* ino = &fs->itable[*ino_ind];
  free(ino_ind);
  
  st->st_mode = ino->i_mode;
  st->st_nlink = ino->i_nlink;
  st->st_size = ino->i_size;
  st->st_mtim = ino->i_mtime;
  
  uint32_t blocks = div_round_up(ino->i_size, VSFS_BLOCK_SIZE);
  if (ino->i_indirect != 0)
    blocks++;
  st->st_blocks = (blocks * VSFS_BLOCK_SIZE) / 512;
  return 0;
}

/**
 * Same as readdir but on a block.
 */
static int
vsfs_read_block(vsfs_blk_t blk, void* buf, fuse_fill_dir_t filler)
{
  fs_ctx *fs = get_fs();
  vsfs_dentry* blk_ptr = (vsfs_dentry*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    if (blk_ptr[i].ino != VSFS_INO_MAX)
      if (filler(buf, blk_ptr[i].name, NULL, 0))
        return -ENOMEM;
  }
  return 0;
}

/**
 * Same as readdir but on an indirect block.
 */
static int
vsfs_read_indirect_block(vsfs_blk_t blk, void* buf, fuse_fill_dir_t filler)
{
  fs_ctx *fs = get_fs();
  vsfs_blk_t* sub_blks = (vsfs_blk_t*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_ino_t); i++) {
    if (sub_blks[i] != 0 && vsfs_read_block(sub_blks[i], buf, filler))
        return -ENOMEM;
  }
  return 0;
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
  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  int err = path_lookup(path, ino_ind);
  assert(!err);
  vsfs_inode* ino = &fs->itable[*ino_ind];
  for (uint32_t i = 0; i < VSFS_NUM_DIRECT; i++) {
    if (ino->i_direct[i] != 0 && vsfs_read_block(ino->i_direct[i], buf, filler))
      return -ENOMEM;
  }
  if (ino->i_indirect != 0)
    return vsfs_read_indirect_block(ino->i_indirect, buf, filler);
  return 0;
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
 * Finds empty dentry in block.
 * @return 0 on success, -1 on error.
 */
static int
find_empty_in_block(vsfs_blk_t blk, vsfs_dentry** dentry_ptr) {
  fs_ctx *fs = get_fs();
  vsfs_dentry* blk_ptr = (vsfs_dentry*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    if (blk_ptr[i].ino == VSFS_INO_MAX) {
      *dentry_ptr = &blk_ptr[i];
      return 0;
    }
  }
  return -1;
}

/**
 * Finds empty dentry in indirect block.
 * @return 0 on success, -1 on error.
 */
static int
find_empty_in_indirect_block(vsfs_blk_t blk, vsfs_dentry** dentry_ptr) {
  fs_ctx *fs = get_fs();
  vsfs_blk_t* sub_blks = (vsfs_blk_t*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_ino_t); i++) {
    if (sub_blks[i] != 0 && find_empty_in_block(sub_blks[i], dentry_ptr) == 0)
        return 0;
  }
  return -1;
}

/**
 * Finds empty dentry in dir.
 * @return 0 on success, -1 on error.
 */
static int
find_empty_dentry(vsfs_ino_t dir, vsfs_dentry** dentry_ptr) {
  fs_ctx *fs = get_fs();
  vsfs_inode* ino = &fs->itable[dir];
  for (uint32_t i = 0; i < VSFS_NUM_DIRECT; i++) {
    if (ino->i_direct[i] != 0 && find_empty_in_block(ino->i_direct[i], dentry_ptr) == 0)
      return 0;
  }
  if (ino->i_indirect != 0) 
    return find_empty_in_indirect_block(ino->i_indirect, dentry_ptr);
  return -1;
}

/*
 * Put blk in indir_blk.
 * @return 0 on success, -1 on failure.
 */
static int
put_block_in_indirect(vsfs_blk_t indir_blk, vsfs_blk_t blk) {
  fs_ctx *fs = get_fs();
  vsfs_blk_t* sub_blks = (vsfs_blk_t*) (fs->image + indir_blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_ino_t); i++) {
    if (sub_blks[i] == 0) {
      sub_blks[i] = blk;
      return 0;
    }
  }
  return -1;
}

/*
 * Allocate indirect block to inode.
 */
void
alloc_indirect_block(vsfs_ino_t ino_ind, vsfs_blk_t blk) {
  fs_ctx* fs = get_fs();
  vsfs_inode* ino = &fs->itable[ino_ind];
  assert(ino->i_indirect == 0);
  ino->i_indirect = blk;
  ino->i_blocks++; // TODO does it count
  ino->i_size += VSFS_BLOCK_SIZE;
  memset(fs->image + ino->i_indirect * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
  bitmap_set(fs->dbmap, fs->sb->num_inodes, ino->i_indirect, true);
  fs->sb->free_blocks--;
}

/*
 * Allocate new block to inode.
 * @return 0 on success, -1 on failure.
 */
static int
alloc_block(vsfs_ino_t ino_ind, vsfs_blk_t blk) {
  fs_ctx* fs = get_fs();
  vsfs_inode* ino = &fs->itable[ino_ind];
  for (uint32_t i = 0; i < VSFS_NUM_DIRECT; i++) {
    if (ino->i_direct[i] == 0) {
      ino->i_direct[i] = blk;
      bitmap_set(fs->dbmap, fs->sb->num_inodes, blk, true);
      ino->i_blocks++;
      fs->sb->free_blocks--;
      return 0;
    }
  }

  if (ino->i_indirect == 0) { // doesn't have indirect block
    vsfs_blk_t* indir_blk = malloc(sizeof(vsfs_blk_t));
    if (bitmap_alloc(fs->dbmap, fs->sb->num_inodes, indir_blk)) {
      free(indir_blk);
      return -1;
    }
    alloc_indirect_block(ino_ind, *indir_blk);
    free(indir_blk);
  }
  if (put_block_in_indirect(ino->i_indirect, blk))
    return -1; // doesn't happen if newly allocated indirect block
  bitmap_set(fs->dbmap, fs->sb->num_inodes, blk, true);
  ino->i_blocks++;
  fs->sb->free_blocks--;
  return 0;
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
  assert(path[0] == '/');
  fs_ctx* fs = get_fs();

  // find inode
  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  if (bitmap_alloc(fs->ibmap, fs->sb->num_inodes, ino_ind)) {
    free(ino_ind);
    return -ENOSPC;
  }
  // find dentry
  vsfs_dentry** dentry_ptr = malloc(sizeof(vsfs_dentry*));
  if (find_empty_dentry(VSFS_ROOT_INO, dentry_ptr)) {
    vsfs_blk_t* new_blk = malloc(sizeof(vsfs_blk_t));
    if (bitmap_alloc(fs->dbmap, fs->sb->num_inodes, new_blk) || alloc_block(VSFS_ROOT_INO, *new_blk)) {
      free(ino_ind);
      free(new_blk);
      return -ENOSPC;
    }
    int err = find_empty_in_indirect_block(*new_blk, dentry_ptr);
    assert(!err);
  }
  // from here we know there is no space error
  bitmap_set(fs->ibmap, fs->sb->num_inodes, *ino_ind, true);
  fs->sb->free_inodes--;
  // initialize inode
  vsfs_inode* ino = &fs->itable[*ino_ind];
  ino->i_mode = mode;
  ino->i_nlink = 1;
  ino->i_blocks = 0; // TODO do we give empty directories block
  ino->i_size = 0;
  int err = clock_gettime(CLOCK_REALTIME, &ino->i_mtime);
  assert(!err);
  for (uint32_t i = 0; i < VSFS_NUM_DIRECT; i++)
    ino->i_direct[i] = 0;
  ino->i_indirect = 0;

  // add inode to directory
  vsfs_dentry* dentry = *dentry_ptr;
  dentry->ino = *ino_ind;
  strcpy(dentry->name, path + 1); 

  free(ino_ind); 
  free(dentry_ptr);
  return 0;
}

/* Free block. */
void
free_block (vsfs_blk_t blk) {
  fs_ctx* fs = get_fs();
  bitmap_free(fs->dbmap, fs->sb->num_inodes, blk);
  fs->sb->free_blocks++;
}

/* Free indirect block and all blocks in it. */
void
free_indirect_block (vsfs_blk_t blk) {
  fs_ctx *fs = get_fs();
  vsfs_blk_t* sub_blks = (vsfs_blk_t*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_ino_t); i++) {
    if (sub_blks[i] != 0) 
      free_block(sub_blks[i]);
  }
  free_block(blk);
}

/**
 * Remove ino from block.
 * @return 0 on success, -1 on failure.
 */
static int
remove_from_block(vsfs_blk_t blk, vsfs_ino_t ino) {
  fs_ctx *fs = get_fs();
  vsfs_dentry* blk_ptr = (vsfs_dentry*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_dentry); i++) {
    if (blk_ptr[i].ino == ino) {
      blk_ptr[i].ino = VSFS_INO_MAX;
      blk_ptr[i].name[0] = '\0';
      return 0;
    }
  }
  return -1;
}

/**
 * Remove ino from indirect block.
 * @return 0 on success, -1 on failure.
 */
static int
remove_from_indirect_block(vsfs_blk_t blk, vsfs_ino_t ino) {
  fs_ctx *fs = get_fs();
  vsfs_blk_t* sub_blks = (vsfs_blk_t*) (fs->image + blk * VSFS_BLOCK_SIZE);
  for (uint32_t i = 0; i < VSFS_BLOCK_SIZE / sizeof(vsfs_ino_t); i++) {
    if (sub_blks[i] != 0 && remove_from_block(sub_blks[i], ino) == 0) 
      return 0;
  }
  return -1;
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
  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  int err = path_lookup(path, ino_ind);
  assert(!err);
  vsfs_inode* ino = &fs->itable[*ino_ind];
  assert(S_ISREG(ino->i_mode) && ino->i_nlink != 0);
 
  // unlink & remove if no links
  ino->i_nlink--;
  if (ino->i_nlink == 0) {
    for (uint32_t i = 0; i < VSFS_NUM_DIRECT; i++) {
      if (ino->i_direct[i] != 0)
        free_block(ino->i_direct[i]);
    }
    if (ino->i_indirect != 0)
      free_indirect_block(ino->i_indirect);
    bitmap_free(fs->ibmap, fs->sb->num_inodes, *ino_ind);
    fs->sb->free_inodes++;
  }

  // remove from directory
  vsfs_inode* dir = &fs->itable[VSFS_ROOT_INO]; 
  for (int i = 0; i < VSFS_NUM_DIRECT; i++) {
    if (dir->i_direct[i] != 0 && remove_from_block(dir->i_direct[i], *ino_ind) == 0) {
      free(ino_ind);
      return 0;
    }
  }
  assert(dir->i_indirect != 0);
  err = remove_from_indirect_block(dir->i_indirect, *ino_ind);
  assert(!err);
  free(ino_ind);
  return 0;
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

  // update the modification timestamp (mtime) in the inode for given
  // path with either the time passed as argument or the current time,
  // according to the utimensat man page

  // 0. Check if there is actually anything to be done.
  if (times[1].tv_nsec == UTIME_OMIT) {
    // Nothing to do.
    return 0;
  }

  // 1. Find the inode for the final component in path
  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  int err = path_lookup(path, ino_ind);
  assert(!err);
  vsfs_inode* ino = &fs->itable[*ino_ind];
  free(ino_ind);

  // 2. Update the mtime for that inode.
  //    This code is commented out to avoid failure until you have set
  //    'ino' to point to the inode structure for the inode to update.
  if (times[1].tv_nsec == UTIME_NOW) {
    if (clock_gettime(CLOCK_REALTIME, &(ino->i_mtime)) != 0) {
    // clock_gettime should not fail, unless you give it a
    // bad pointer to a timespec.
      assert(false);
    }
  } else {
    ino->i_mtime = times[1];
  }

  return 0;
}

/**
 * Find byte at offset in indir_blk.
 * Offset starts at the beginning of the first block in indir_blk.
 * @return 0 on success; -1 on error.
 */
static int
seek_in_indirect_block(vsfs_blk_t indir_blk, uint64_t offset, vsfs_blk_t* blk, uint64_t* offset_in_blk) {
  uint32_t blk_cnt = offset / VSFS_BLOCK_SIZE;
  if (blk_cnt >= VSFS_BLOCK_SIZE / sizeof(vsfs_ino_t))
    return -1;
  fs_ctx *fs = get_fs();
  vsfs_blk_t* sub_blks = (vsfs_blk_t*) (fs->image + indir_blk * VSFS_BLOCK_SIZE);
  if (sub_blks[blk_cnt] == 0) // TODO: assuming in order w no holes?
    return -1;
  *blk = sub_blks[blk_cnt];
  *offset_in_blk = offset % VSFS_BLOCK_SIZE;
  return 0;
}

/**
 * Find byte at offset in ino. 
 * @return 0 on success; -1 on error.
 */
static int
seek_in_file(vsfs_ino_t ino_ind, uint64_t offset, vsfs_blk_t* blk, uint64_t* offset_in_blk) {
  assert(offset < VSFS_MAX_FILE_SIZE);
  fs_ctx* fs = get_fs();
  vsfs_inode* ino = &fs->itable[ino_ind];
  uint64_t blk_num = offset / VSFS_BLOCK_SIZE;
  if (blk_num < VSFS_NUM_DIRECT) {
    if (ino->i_direct[blk_num] == 0)
      return -1;
    *blk = ino->i_direct[blk_num];
  } else if (ino->i_indirect == 0 || seek_in_indirect_block(ino->i_indirect, offset - VSFS_BLOCK_SIZE * VSFS_NUM_DIRECT, blk, offset_in_blk))
      return -1;
  *offset_in_blk = offset % VSFS_BLOCK_SIZE;
  return 0;
}

/**
 Truncate the number of blocks to match new_blks.
 */
static int
truncate_blocks(vsfs_ino_t ino_ind, uint64_t new_blks) {
  fs_ctx* fs = get_fs();
  vsfs_inode* ino = &fs->itable[ino_ind];
  uint64_t old_blks = div_round_up(ino->i_size, VSFS_BLOCK_SIZE);
  if (new_blks == old_blks) {
    return 0;
  } else if (new_blks < old_blks) {
    for (uint64_t i = new_blks; i < old_blks && i < VSFS_NUM_DIRECT; i++) {
        assert(ino->i_direct[i] != 0);
        free_block(ino->i_direct[i]);
        ino->i_direct[i] = 0;
      }
      if (new_blks >= VSFS_NUM_DIRECT) {
        assert(old_blks >= VSFS_NUM_DIRECT && ino->i_indirect != 0);
        uint64_t keep_blks = new_blks - VSFS_NUM_DIRECT;
        memset(fs->image + ino->i_indirect * VSFS_BLOCK_SIZE + keep_blks * sizeof(vsfs_blk_t), 0, VSFS_BLOCK_SIZE - keep_blks * sizeof(vsfs_blk_t));
      } else if (old_blks >= VSFS_NUM_DIRECT) {
        assert(ino->i_indirect != 0);
        free_indirect_block(ino->i_indirect);
        ino->i_indirect = 0;
      }
  } else {
    // find necessary blocks
      vsfs_blk_t* blks = malloc(sizeof(vsfs_blk_t) * (new_blks - old_blks));
      for (uint64_t i = 0; i < new_blks - old_blks; i++) {
        if (bitmap_alloc(fs->dbmap, fs->sb->num_inodes, &blks[i])) {
          free(blks);
          return -ENOSPC;
        }
      }
      // add indirect block if necessary
      if (new_blks >= VSFS_NUM_DIRECT && ino->i_indirect == 0) {
        vsfs_blk_t* indir_blk = malloc(sizeof(vsfs_blk_t));
        if (bitmap_alloc(fs->dbmap, fs->sb->num_inodes, indir_blk)) {
          free(blks);
          free(indir_blk);
          return -ENOSPC;
        }
        alloc_indirect_block(ino_ind, *indir_blk);
        free(indir_blk);
      }
      // allocate the blocks
      for (uint64_t i = 0; i < new_blks - old_blks; i++) {
        int err = alloc_block(ino_ind, blks[i]);
        assert(!err);
        memset(fs->image + blks[i] * VSFS_BLOCK_SIZE, 0, VSFS_BLOCK_SIZE);
      }
      free(blks);
  }
  ino->i_blocks = new_blks;
  return 0;
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
  assert (size >= 0);
  if ((uint64_t) size >= VSFS_MAX_FILE_SIZE)
    return -EFBIG;

  fs_ctx* fs = get_fs();
  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  int err = path_lookup(path, ino_ind);
  assert(!err);
  vsfs_inode* ino = &fs->itable[*ino_ind];

  // truncate blocks
  // TODO make sure 32 and 64 is correct
  uint64_t new_blks = div_round_up(size, VSFS_BLOCK_SIZE);
  err = truncate_blocks(*ino_ind, new_blks);
  if (err) {
    free(ino_ind);
    return err;
  }

  // truncate boundary block (only need to set size & sometimes pad with zeros)
  if ((uint64_t) size > ino->i_size) {
    vsfs_blk_t* blk = malloc(sizeof(vsfs_blk_t));
    uint64_t* offset = malloc(sizeof(uint64_t));
    int err = seek_in_file(*ino_ind, ino->i_size, blk, offset);
    assert(!err);
    memset(fs->image + *blk * VSFS_BLOCK_SIZE + *offset, 0, VSFS_BLOCK_SIZE - *offset);
    free(blk);
    free(offset);
  }
  ino->i_size = size;
  free(ino_ind);
  return 0;
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
  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  int err = path_lookup(path, ino_ind);
  assert(!err);
  vsfs_inode* ino = &fs->itable[*ino_ind];  
 
  if ((uint64_t) offset >= ino->i_size) {
    memset(buf, 0, size);
    free(ino_ind);
    return 0;
  }
 
  vsfs_blk_t* blk = malloc(sizeof(vsfs_blk_t));
  uint64_t* offset_in_blk = malloc(sizeof(uint64_t));
  err = seek_in_file(*ino_ind, offset, blk, offset_in_blk); 
  assert(!err);
  void* ptr = (fs->image + *blk * VSFS_BLOCK_SIZE + *offset_in_blk);

  int ret = (offset + size <= ino->i_size) ? size : ino->i_size - offset;
  memcpy(buf, ptr, ret);
  memset(buf + ret, 0, size - ret);
  
  free(ino_ind);
  free(blk);
  free(offset_in_blk);
  return ret;
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
  if (offset + size > VSFS_MAX_FILE_SIZE)
    return -EFBIG; // TODO do i write however much i can?

  fs_ctx* fs = get_fs();
  vsfs_ino_t* ino_ind = malloc(sizeof(vsfs_ino_t));
  int err = path_lookup(path, ino_ind);
  assert(!err);
  vsfs_inode* ino = &fs->itable[*ino_ind];  

  if (offset + size > ino->i_size) {
    // truncate blocks
    uint64_t new_blks = div_round_up(offset + size, VSFS_BLOCK_SIZE);
    err = truncate_blocks(*ino_ind, new_blks);
    if (err) {
      free(ino_ind);
      return err;
    }
    // pad boundary block with zeros
    vsfs_blk_t* blk = malloc(sizeof(vsfs_blk_t));
    uint64_t* offset_in_blk = malloc(sizeof(uint64_t));
    int err = seek_in_file(*ino_ind, ino->i_size, blk, offset_in_blk);
    assert(!err);
    memset(fs->image + *blk * VSFS_BLOCK_SIZE + *offset_in_blk, 0, VSFS_BLOCK_SIZE - *offset_in_blk);
    free(blk);
    free(offset_in_blk);

    ino->i_size = offset + size;
  }

  vsfs_blk_t* blk = malloc(sizeof(vsfs_blk_t));
  uint64_t* offset_in_blk = malloc(sizeof(uint64_t));
  err = seek_in_file(*ino_ind, offset, blk, offset_in_blk); 
  assert(!err);
  void* ptr = (fs->image + *blk * VSFS_BLOCK_SIZE + *offset_in_blk);
  memcpy(ptr, buf, size);
  
  free(ino_ind);
  free(blk);
  free(offset_in_blk);
  return size;
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
