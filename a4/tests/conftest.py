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


def pytest_addoption(parser):
    """Add the mount_point, inode_count, and disk command line arguments."""
    parser.addoption('--mount_point', action='store', type=str)
    parser.addoption('--inode_count', action='store', type=int)
    parser.addoption('--disk', action='store', type=str)


@pytest.fixture(scope='session')
def mount_point(request) -> str:
    """Extract the mount_point argument from the command line.

    Using mount_point as a parameter name for a test will give you the value given at the command line. If that value
    was not given on the command line, then any tests that use this as a parameter name will be skipped.
    """
    given_mount_point = request.config.option.mount_point
    if given_mount_point is None:
        pytest.skip()
    return given_mount_point


@pytest.fixture(scope='session')
def inode_count(request) -> int:
    """Extract the inode_count argument from the command line.

    Using inode_count as a parameter name for a test will give you the value given at the command line. If that value
    was not given on the command line, then any tests that use this as a parameter name will be skipped.
    """
    given_inode_count = request.config.option.inode_count
    if given_inode_count is None:
        pytest.skip()
    return given_inode_count


@pytest.fixture(scope='session')
def disk(request) -> str:
    """Extract the disk argument from the command line and return the "basename".

    For example, if the argument was: '/path/to/my.disk', then this will return 'my.disk'.

    Using disk as a parameter name for a test will give you the value given at the command line. If that value
    was not given on the command line, then any tests that use this as a parameter name will be skipped.
    """
    given_disk = request.config.option.disk
    if given_disk is None:
        pytest.skip()
    return os.path.basename(given_disk)
