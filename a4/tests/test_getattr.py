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

import expected_values


def test_st_mode(mount_point: str, disk: str) -> None:
    """Test that every file in the filesystem has the expected file type and mode (i.e., st_mode).

    This test is only useful if disk is in expected_values.STATVFS.
    """
    assert disk in expected_values.STATVFS, f'The disk argument {disk} does not have any expected values.'

    for file in expected_values.FILES[disk]:
        assert file in expected_values.FILE_STATS, f'Could not find stats for {file}'

        path_to_file = os.path.join(mount_point, file)
        actual = os.stat(path_to_file)

        expected = expected_values.FILE_STATS[file]
        assert actual.st_mode == expected.st_mode


# TODO: Add more tests!
# Tips
#   - The "easy" tests to write compare actual with expected. These show "fast failures", but don't provide significant
#       insight.
#   - Consider adding "useful" tests that use your domain knowledge of file systems to help find bugs. For example,
#       based on the expected size of a file, can you write a test that is more specific about which blocks *might*
#       have not been included in st_blocks when it does not contain the expected value?
