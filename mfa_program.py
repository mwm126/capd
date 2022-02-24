#!/usr/bin/env python

import base64
import getpass
import hashlib
import hmac
import os
import subprocess
import sys
import time

from argparse import ArgumentParser, Namespace
from pathlib import Path
from typing import Tuple

from OpenSSL import crypto

# Global Variables (secrets)
hashIDSalt = "4FEGy8Vb2ih8fRZow2qK2QE32BYe0ZffTtEmj1QI0sJ5l7HOMPcm4G+F"
hmacGenerator = base64.b64decode("02VDv98CjuUVnjDOv1ySVB+YbEFISOUUW6Lgc2UGCjuSQJHzRhMLfKP3vFkMikSp")
otpGenerator = base64.b64decode("EdnkfUYMK8yl1XxJPibr60g6laGvpk7tBPVMCCyxpam5dQyziBx7K5DdraA2c6KC")
pinGenerator = base64.b64decode("bseTuIXxT5E+Z7zKZlHnRzmv8J5/UY315O08z+N/LL7Z1LRku9ocQJbKjYgNMURz")
pukGenerator = base64.b64decode("/7jZypeK4aQqBFZvecVrgRep+ZBIEiu4TQP4lQtamYbdV0UJgvaEMsTYOG4bylX4")
mfaAccess = "83b9fe94a405"
MGMKEY = "e1a055640fd1b0ef5eb5b9dfd5c867d3e035bf2885ca5276"


def main(args: list[str]):
    print("\nMFA4 SciLAN/HPC Programmer\n")

    parser = get_parser()
    ns = parser.parse_args(args)

    if ns.command:
        ns.func(ns)
    else:
        parser.print_help()


def get_parser() -> ArgumentParser:
    parser = ArgumentParser(description="CAP Client")
    parser.set_defaults(func=parser.print_usage)

    subpar = parser.add_subparsers(dest="command")

    ca = subpar.add_parser("ca", help="Create Certificate Authority (CA) key pair")
    ca.set_defaults(func=create_key_pair)

    yk = subpar.add_parser("yk", help="Program Yubikey with CA key pair")
    yk.set_defaults(func=program_user_key)

    ca.add_argument(
        "-p",
        "--private",
        metavar="<private key>",
        action="store",
        default="CAPrivateKey.pem",
        help="Path to CA private key",
    )
    ca.add_argument(
        "-t",
        "--cert",
        metavar="<certificate>",
        action="store",
        default="CACertificate.pem",
        help="Path to CA certificate",
    )

    yk.add_argument(
        "-u",
        "--username",
        metavar="<username>",
        action="store",
        default="",
        help="Username for key to program",
    )
    yk.add_argument(
        "-l",
        "--log",
        action="store",
        default="mfaKey.log",
        help="Path to log file",
    )
    yk.add_argument(
        "-p",
        "--private",
        metavar="<private key>",
        action="store",
        default="CAPrivateKey.pem",
        help="Path to CA private key",
    )
    yk.add_argument(
        "-t",
        "--cert",
        metavar="<certificate>",
        action="store",
        default="CACertificate.pem",
        help="Path to CA certificate",
    )
    yk.add_argument(
        "-o",
        "--once",
        action="store_true",
        help="Only program one user key",
    )

    return parser


def create_key_pair(ns: Namespace):
    """Handle Key Pair Creation"""
    cert_p = Path(ns.cert)
    privkey_p = Path(ns.private)
    print("Preparing to create a new Certificate Authority key pair.")
    if cert_p.exists():
        yn = input(f"Certificate already exists at {cert_p}.  Overwrite?")
        if yn.lower() not in ["y", "yes"]:
            print("\nExiting.\n")
            return
    if privkey_p.exists():
        yn = input(f"Private key already exists at {privkey_p}.  Overwrite?")
        if yn.lower() not in ["y", "yes"]:
            print("\nExiting.\n")
            return
    print("\nCreating RSA4096 key pair for Certificate Authority.")
    CAKeyPair = crypto.PKey()
    CAKeyPair.generate_key(crypto.TYPE_RSA, 4096)

    while True:
        privatePass = getpass.getpass("\nEnter passphrase for CA Private Key: ")
        if privatePass == getpass.getpass("Re-enter passphrase: "):
            break
        print("Passphases do not match.")

    print("\nCreating CA Certificate.")
    CACert = crypto.X509()
    CACert.set_pubkey(CAKeyPair)
    CACert.set_serial_number(7)
    CACert.gmtime_adj_notBefore(0)
    CACert.gmtime_adj_notAfter(50 * 365 * 24 * 60 * 60)
    CACert.get_subject().C = "US"
    CACert.get_subject().O = "USDOE-NETL"
    CACert.get_subject().CN = "SciLAN-HPC Admin"
    CACert.set_issuer(CACert.get_subject())
    CACert.set_pubkey(CAKeyPair)
    CACert.sign(CAKeyPair, "sha512")

    # verify CA cert
    print("Verifying CA Certificate.")
    store = crypto.X509Store()
    store.add_cert(CACert)
    store_ctx = crypto.X509StoreContext(store, CACert)
    try:
        store_ctx.verify_certificate()
    except crypto.X509StoreContextError:
        print("CA Certificate verification failure.  Exiting.")
        sys.exit()
    print("CA Certificate verification completed.")

    print(f"\nWriting CA Certificate to {cert_p}.")
    with open(cert_p, "wb") as CACertFile:
        CACertFile.write(crypto.dump_certificate(crypto.FILETYPE_PEM, CACert))

    print(f"Writing Encrypted CA Private Key to {privkey_p}.")

    priv_key = crypto.dump_privatekey(
        crypto.FILETYPE_PEM,
        CAKeyPair,
        "aes-256-cbc",
        bytes(privatePass, "utf-8"),
    )

    with open(privkey_p, "wb") as CAPrivateKeyFile:
        CAPrivateKeyFile.write(priv_key)
    print("Exiting.\n")


def program_user_key(ns: Namespace):
    """User Key Creation and Yubikey Programming"""

    # Load and decrypt CA certificate and private key
    privatePass = getpass.getpass("Enter passphrase for CA Private Key: ")
    try:
        with open(ns.private, "rb") as CAPrivateKeyFile:
            CAPrivateKey = crypto.load_privatekey(
                crypto.FILETYPE_PEM,
                CAPrivateKeyFile.read(),
                bytes(privatePass, "utf-8"),
            )
    except ValueError:
        print("Error opening CA Key.")
        print("Either the key file cannot be found or passphrase is incorrect.")
        print("Run with -c option to create a new CA key pair.")
        print("Exiting.")
        sys.exit()
    try:
        with open(ns.cert, "rb") as CACertFile:
            CACert = crypto.load_certificate(crypto.FILETYPE_PEM, CACertFile.read())
    except ValueError:
        print("Error opening CA Certificate.")
        print("Run with -c option to create a new CA key pair.")
        print("Exiting.")
        sys.exit()
    print("CA Key successfully unlocked.")

    # Forever loop for user creation without re-entering passphrase
    while True:
        # prompt for new username/exit
        print("\nInsert new Yubikey 4 and enter username or press enter to exit: ")
        username = input("Username: ")
        if username == "":
            print("Exiting.")
            sys.exit()

        # get serial number from yubikey
        try:
            serialText = subprocess.check_output(["ykinfo", "-sq"])
            serial = int(serialText)
        except ValueError:
            print("Problem detecting Yubikey.")
            continue
        if serial < 4000000:
            print("Yubikey inserted is not a Yubikey 4")
            sys.exit()
        print("Found Yubikey 4 with serial number: ", serial)

        # reset the key
        print("\nResetting yubikey.")

        ykPers("-y -z -1 -c" + mfaAccess)
        ykPers("-y -z -2 -c" + mfaAccess)

        for _ in range(4):
            pivTool("-a verify-pin -P 1001")
        for _ in range(4):
            pivTool("-a verify-pin -P 1002")
        for _ in range(4):
            pivTool("-a change-puk -P 1003 -N 1000001")
        for _ in range(4):
            pivTool("-a change-puk -P 1003 -N 1000002")
        pivTool("-a reset")

        # generate user's secrets
        hashID = sha256(username + str(serial) + hashIDSalt)
        userHmac: bytes = sha1(hashID + hmacGenerator)
        userOtp = sha1(hashID + otpGenerator)[0:16]

        # program the keys with hmac secrets
        print("\nProgramming OTP secret in Slot 1.")
        command = "-y -1 -c" + mfaAccess + " -ochal-resp -ochal-yubico "
        command += " -a" + userOtp.hex() + " -oaccess=" + mfaAccess
        command += " -oserial-usb-visible -oserial-api-visible"
        ykPers(command)
        chal = os.urandom(63)
        keyResp, _ = ykChalResp("-1 -Y -x " + chal.hex())

        #  h = hmac.new(userOtp,chal,hashlib.sha1)
        #  chkResp = h.digest().encode('hex')
        #  if keyResp.strip() != chkResp:
        #    print "Slot 1 OTP test failure.  Exiting."
        #    sys.exit()
        #  print "OTP Slot 1 test complete."

        print("\nProgramming HMAC secret in Slot 2.")
        command = "-y -2 -c" + mfaAccess + " -ochal-resp -ochal-hmac -ohmac-lt64"
        command += " -a" + userHmac.hex() + " -oaccess=" + mfaAccess
        command += " -oserial-usb-visible -oserial-api-visible"
        ykPers(command)
        chal = os.urandom(63)
        keyResp, _ = ykChalResp("-2 -H -x " + chal.hex())
        h = hmac.new(userHmac, chal, hashlib.sha1)
        chkResp = h.digest().hex().encode("utf-8")
        if keyResp.strip() != chkResp:
            print(keyResp.strip(), chkResp)
            print("Slot 2 HMAC test failure.  Exiting.")
            sys.exit()
        print("HMAC Slot 2 test complete.")

        # generate user's pin/puk
        keyPIN = base64.b64encode(sha256(str(serial) + str(pinGenerator))).decode("utf-8")[0:8]
        keyPUK = base64.b64encode(sha256(str(serial) + str(pukGenerator))).decode("utf-8")[0:8]
        print("\nSetting yubikey configuration data.")
        try:
            print(pivTool("-a set-chuid"))
            print(pivTool("-a set-ccc"))
            print(pivTool("-a set-mgm-key -n" + MGMKEY))
            print(pivTool("-a change-pin -P 123456 -N" + keyPIN))
            print(pivTool("-a change-puk -P 12345678 -N" + keyPUK))
        except ValueError:
            sys.exit()

        # Program all three PIV certificate slots
        programRSASlot(MGMKEY, username, keyPIN, "9a", serial, CACert, CAPrivateKey)
        programRSASlot(MGMKEY, username, keyPIN, "9c", serial, CACert, CAPrivateKey)
        programRSASlot(MGMKEY, username, keyPIN, "9d", serial, CACert, CAPrivateKey)

        print("OTP Secret:  " + userOtp.hex())
        print("HMAC Secret: " + userHmac.hex())

        # Write out to log file
        with open(ns.log, "a", encoding="utf-8") as f:
            out = "[" + time.asctime(time.localtime(time.time())) + "]   "
            out += username + "\t"
            out += str(serial) + "   "
            out += keyPIN + "\n"
            f.write(out)

        print("Key programmed and tested for user " + username + ".  Remove key.")

        if ns.once:
            break


def sha256(string: str) -> bytes:
    m = hashlib.sha256()
    m.update(string.encode("utf-8"))
    return m.digest()


def sha1(string: bytes) -> bytes:
    m = hashlib.sha1()
    m.update(string)
    return m.digest()


def run(cmd: str, args: str) -> Tuple[bytes, bytes]:
    argList = [cmd] + args.split()
    with subprocess.Popen(argList, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as p:
        return p.communicate()


def pivTool(args: str) -> str:
    return run("yubico-piv-tool", args)[1].decode("utf-8")


def ykPers(args: str) -> Tuple[bytes, bytes]:
    return run("ykpersonalize", args)


def ykChalResp(args: str):
    return run("ykchalresp", args)


def programRSASlot(
    mgmKey: str,
    username: str,
    keyPIN: str,
    slot: str,
    serial: int,
    CACert: crypto.X509,
    CAPrivateKey: crypto.PKey,
) -> None:
    # make a new public key for the user
    userKeyPair = crypto.PKey()
    userKeyPair.generate_key(crypto.TYPE_RSA, 2048)

    userCert = crypto.X509()
    userCert.set_serial_number(serial)
    userCert.gmtime_adj_notBefore(0)
    userCert.gmtime_adj_notAfter(50 * 365 * 24 * 60 * 60)
    userCert.get_subject().C = "US"
    userCert.get_subject().O = "USDOE-NETL"
    userCert.get_subject().CN = username
    userCert.set_pubkey(userKeyPair)
    userCert.set_issuer(CACert.get_subject())
    userCert.sign(CAPrivateKey, "sha512")

    # verify user cert
    store = crypto.X509Store()
    store.add_cert(CACert)
    store_ctx = crypto.X509StoreContext(store, userCert)
    try:
        store_ctx.verify_certificate()
    except crypto.X509StoreContextError:
        print("User Certificate verification failure.  Exiting.")
        sys.exit()
    print("User Certificate for slot " + slot + " created and verified.")

    print("Pushing user certificate to yubikey, slot " + slot + ".")
    userCertPem = crypto.dump_certificate(crypto.FILETYPE_PEM, userCert).decode("utf-8")
    import_cert_out = subprocess.check_output(
        ["yubico-piv-tool", f"--key={mgmKey}", "-s", slot, "-a", "import-certificate"],
        encoding="utf-8",
        input=userCertPem,
    )
    print(import_cert_out)

    userCertPem2 = subprocess.check_output(
        ["yubico-piv-tool", "-s", slot, "-a", "read-certificate"], encoding="utf-8"
    )

    if userCertPem != userCertPem2:
        print("User Certificate on key not verified.  Exiting.")
        sys.exit()
    print("User Certificate on key is verified.")

    print("Pushing user private key to yubikey, slot " + slot + ".")
    priv_key = crypto.dump_privatekey(crypto.FILETYPE_PEM, userKeyPair).decode("utf-8")
    import_out = subprocess.check_output(
        ["yubico-piv-tool", f"--key={mgmKey}", "-s", slot, "-a", "import-key"],
        input=priv_key,
        encoding="utf-8",
    )
    print(import_out)

    print("Verifying RSA2048 signature with key in slot " + slot + ".")
    verify_pin_out = subprocess.check_output(
        [
            "yubico-piv-tool",
            "-a",
            "verify-pin",
            "-P",
            keyPIN,
            "-s",
            slot,
            "-a",
            "test-signature",
        ],
        input=userCertPem2,
        encoding="utf-8",
    )
    print(verify_pin_out)


if __name__ == "__main__":
    main(sys.argv[1:])
