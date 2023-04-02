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

import os
import pytest

import expected_values

CSC369_A4_BLOCK_SIZE = 4096
CSC369_A4_MAX_FILE_LENGTH = 252


@pytest.fixture()
def stats(mount_point: str) -> os.statvfs_result:
    """Return the filesystem statistics obtained by a statvfs call on mount_point."""
    return os.statvfs(mount_point)


def test_f_bsize(stats: os.statvfs_result) -> None:
    """Test that filesystem has the expected block size."""
    assert stats.f_bsize == CSC369_A4_BLOCK_SIZE


def test_f_frsize(stats: os.statvfs_result) -> None:
    """Test that filesystem has the expected fragment size."""
    assert stats.f_frsize == CSC369_A4_BLOCK_SIZE


def test_f_files(stats: os.statvfs_result, inode_count: int) -> None:
    """Test that filesystem has the expected number of inodes."""
    assert stats.f_files == inode_count


def test_f_namemax(stats: os.statvfs_result) -> None:
    """Test that filesystem has the expected maximum filename length."""
    assert stats.f_namemax == CSC369_A4_MAX_FILE_LENGTH


def test_every_statvfs_attribute(stats: os.statvfs_result, disk: str) -> None:
    """Test that filesystem has the expected value for every attribute in a statvfs result.

    This test is only useful if disk is in expected_values.STATVFS.
    """
    assert disk in expected_values.STATVFS, 'The disk argument does not have any expected values.'

    expected = expected_values.STATVFS[disk]
    assert stats == expected
