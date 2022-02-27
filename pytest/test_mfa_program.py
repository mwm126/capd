import os
from pathlib import Path
from unittest.mock import patch

import pytest

import mfa_program


def test_default():
    mfa_program.main([])


@patch("mfa_program.input")
@patch("mfa_program.getpass")
def test_make_cert_default(mock_getpass, mock_input, tmpdir):
    mock_getpass.getpass.return_value = "yes"
    mock_input.return_value = "yes"
    # p = tmpdir.mkdir()
    default_cert = tmpdir / "CACertificate.pem"
    default_key = tmpdir / "CAPrivateKey.pem"
    assert not default_cert.exists()
    assert not default_key.exists()

    with tmpdir.as_cwd() as _:
        mfa_program.main(["ca"])

    assert default_cert.exists()
    assert default_key.exists()


@patch("mfa_program.input")
@patch("mfa_program.getpass")
def test_make_cert_cancel(mock_getpass, mock_input, tmpdir):
    mock_getpass.getpass.return_value = "no"
    mock_input.return_value = "no"
    default_cert = Path(tmpdir / "CACertificate.pem")
    default_key = Path(tmpdir / "CAPrivateKey.pem")
    default_cert.touch()
    default_key.touch()
    assert not default_cert.stat().st_size
    assert not default_key.stat().st_size

    with tmpdir.as_cwd() as _:
        mfa_program.main(["ca"])

    assert not default_cert.stat().st_size
    assert not default_key.stat().st_size


@patch("mfa_program.input")
@patch("mfa_program.getpass")
def test_make_cert_overwrite(mock_getpass, mock_input, tmpdir):
    mock_getpass.getpass.return_value = "yes"
    mock_input.return_value = "yes"
    default_cert = Path(tmpdir / "CACertificate.pem")
    default_key = Path(tmpdir / "CAPrivateKey.pem")
    default_cert.touch()
    default_key.touch()
    assert not default_cert.stat().st_size
    assert not default_key.stat().st_size

    with tmpdir.as_cwd() as _:
        mfa_program.main(["ca"])

    assert default_cert.stat().st_size
    assert default_key.stat().st_size


@patch("mfa_program.input")
@patch("mfa_program.getpass")
def test_make_cert_custom(mock_getpass, mock_input, tmpdir):
    mock_getpass.getpass.return_value = "meh"
    mock_input.return_value = "meh"
    cert = tmpdir / "my_ca.pem"
    key = tmpdir / "my_key.pem"
    assert not cert.exists()
    assert not key.exists()

    with tmpdir.as_cwd() as _:
        mfa_program.main(["ca", "-t", "my_ca.pem", "-p", "my_key.pem"])

    assert cert.exists()
    assert key.exists()


YK_PRESENT = bool(os.system("ykinfo -s"))


@pytest.mark.skipif(YK_PRESENT, reason="Test requires Yubikey [which will be wiped]")
@patch("mfa_program.input")
@patch("mfa_program.getpass")
def test_program_yubikey(mock_getpass, mock_input, tmpdir):
    mock_getpass.getpass.return_value = "meh"
    mock_input.return_value = "meh"

    cert = tmpdir / "CACertificate.pem"
    key = tmpdir / "CAPrivateKey.pem"
    assert not cert.exists()
    assert not key.exists()

    with tmpdir.as_cwd() as _:
        mfa_program.main(["ca"])
        assert cert.exists()
        assert key.exists()

        mfa_program.main(["yk", "--user", "the_user", "--once"])
