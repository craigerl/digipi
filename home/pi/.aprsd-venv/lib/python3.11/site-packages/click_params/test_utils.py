"""Helper functions to test click commands"""
from typing import List

from click.testing import Result


def assert_in_output(exit_code: int, expected_output: str, result: Result) -> None:
    assert exit_code == result.exit_code  # nosec
    assert expected_output in result.output  # nosec


def assert_list_in_output(exit_code: int, data: List[str], result: Result) -> None:
    assert exit_code == result.exit_code  # nosec
    for item in data:
        assert item in result.output  # nosec


def assert_equals_output(exit_code: int, expected_output: str, result: Result) -> None:
    assert exit_code == result.exit_code  # nosec
    assert expected_output == result.output  # nosec
