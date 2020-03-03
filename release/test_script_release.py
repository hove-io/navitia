from script_release import get_release_type
from mock import patch
import pytest


@patch("sys.stdin.readline")
def test_get_correct_release_type_depending_on_question(patch_stdin):

    """
    Question list:
     1 - Do you need binarization ? [Y/n]
     2 - Have you changed the API or Data interface ? [Y/n]
     3 - Are the changes backward compatible ? [Y/n]
     4 - Are you hotfixing ? [Y/n]
    """
    patch_stdin.side_effect = ["y"]
    assert get_release_type() == "major"

    patch_stdin.side_effect = ["Y"]
    assert get_release_type() == "major"

    patch_stdin.side_effect = ["n", "y"]
    assert get_release_type() == "major"

    patch_stdin.side_effect = ["n", "n", "y"]
    assert get_release_type() == "minor"

    patch_stdin.side_effect = ["n", "n", "n", "y"]
    assert get_release_type() == "hotfix"


@patch("sys.stdin.readline")
def test_get_release_type_should_raise_on_wrong_input(patch):
    patch.side_effect = ["this", "is", "really", "wrong"]
    with pytest.raises(Exception):
        get_release_type()
