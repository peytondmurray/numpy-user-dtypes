import numpy as np
import pytest

from asciidtype import ASCIIDType, ASCIIScalar


def test_dtype_creation():
    dtype = ASCIIDType(4)
    assert str(dtype) == "ASCIIDType(4)"


def test_scalar_creation():
    dtype = ASCIIDType(7)
    ASCIIScalar("string", dtype)


def test_creation_with_explicit_dtype():
    dtype = ASCIIDType(7)
    arr = np.array(["hello", "this", "is", "an", "array"], dtype=dtype)
    assert repr(arr) == (
        "array(['hello', 'this', 'is', 'an', 'array'], dtype=ASCIIDType(7))"
    )


def test_creation_truncation():
    inp = ["hello", "this", "is", "an", "array"]

    dtype = ASCIIDType(5)
    arr = np.array(inp, dtype=dtype)
    assert repr(arr) == (
        "array(['hello', 'this', 'is', 'an', 'array'], dtype=ASCIIDType(5))"
    )

    dtype = ASCIIDType(4)
    arr = np.array(inp, dtype=dtype)
    assert repr(arr) == (
        "array(['hell', 'this', 'is', 'an', 'arra'], dtype=ASCIIDType(4))"
    )

    dtype = ASCIIDType(1)
    arr = np.array(inp, dtype=dtype)
    assert repr(arr) == (
        "array(['h', 't', 'i', 'a', 'a'], dtype=ASCIIDType(1))"
    )
    assert arr.tobytes() == b"htiaa"

    # dtype = ASCIIDType()
    # arr = np.array(["hello", "this", "is", "an", "array"], dtype=dtype)
    # assert repr(arr) == ("array(['', '', '', '', ''], dtype=ASCIIDType(0))")
    # assert arr.tobytes() == b""


def test_casting_to_asciidtype():
    for dtype in (None, ASCIIDType(5)):
        arr = np.array(["this", "is", "an", "array"], dtype=dtype)

        assert repr(arr.astype(ASCIIDType(7))) == (
            "array(['this', 'is', 'an', 'array'], dtype=ASCIIDType(7))"
        )

        assert repr(arr.astype(ASCIIDType(5))) == (
            "array(['this', 'is', 'an', 'array'], dtype=ASCIIDType(5))"
        )

        assert repr(arr.astype(ASCIIDType(4))) == (
            "array(['this', 'is', 'an', 'arra'], dtype=ASCIIDType(4))"
        )

        assert repr(arr.astype(ASCIIDType(1))) == (
            "array(['t', 'i', 'a', 'a'], dtype=ASCIIDType(1))"
        )

        # assert repr(arr.astype(ASCIIDType())) == (
        #    "array(['', '', '', '', ''], dtype=ASCIIDType(0))"
        # )


def test_unicode_to_ascii_to_unicode():
    arr = np.array(["hello", "this", "is", "an", "array"])
    ascii_arr = arr.astype(ASCIIDType(5))
    round_trip_arr = ascii_arr.astype("U5")
    np.testing.assert_array_equal(arr, round_trip_arr)


def test_creation_fails_with_non_ascii_characters():
    inps = [
        ["😀", "¡", "©", "ÿ"],
        ["😀", "hello", "some", "ascii"],
        ["hello", "some", "ascii", "😀"],
    ]
    for inp in inps:
        with pytest.raises(
            TypeError,
            match="Can only store ASCII text in a ASCIIDType array.",
        ):
            np.array(inp, dtype=ASCIIDType(5))


def test_casting_fails_with_non_ascii_characters():
    inps = [
        ["😀", "¡", "©", "ÿ"],
        ["😀", "hello", "some", "ascii"],
        ["hello", "some", "ascii", "😀"],
    ]
    for inp in inps:
        arr = np.array(inp)
        with pytest.raises(
            TypeError,
            match="Can only store ASCII text in a ASCIIDType array.",
        ):
            arr.astype(ASCIIDType(5))
