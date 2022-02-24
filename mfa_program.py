#!/usr/bin/env python

import base64
import getpass
import hashlib
import hmac
import os
import subprocess
import sys
import time

from typing import Tuple

from OpenSSL import crypto

# Global Variables
storagePath = "/mnt/mfa/"
ykinfo = "/usr/local/bin/ykinfo"
ykpersonalize = "/usr/local/bin/ykpersonalize"
ykchalresp = "/usr/local/bin/ykchalresp"
yubicoPivTool = "/usr/local/bin/yubico-piv-tool"

# secrets
hashIDSalt = "4FEGy8Vb2ih8fRZow2qK2QE32BYe0ZffTtEmj1QI0sJ5l7HOMPcm4G+F"
hmacGenTxt = "02VDv98CjuUVnjDOv1ySVB+YbEFISOUUW6Lgc2UGCjuSQJHzRhMLfKP3vFkMikSp"
otpGenTxt = "EdnkfUYMK8yl1XxJPibr60g6laGvpk7tBPVMCCyxpam5dQyziBx7K5DdraA2c6KC"
hmacGenerator = base64.b64decode(hmacGenTxt)
otpGenerator = base64.b64decode(otpGenTxt)
pinGenTxt = "bseTuIXxT5E+Z7zKZlHnRzmv8J5/UY315O08z+N/LL7Z1LRku9ocQJbKjYgNMURz"
pinGenerator = base64.b64decode(pinGenTxt)
pukGenTxt = "/7jZypeK4aQqBFZvecVrgRep+ZBIEiu4TQP4lQtamYbdV0UJgvaEMsTYOG4bylX4"
pukGenerator = base64.b64decode(pukGenTxt)
mfaAccess = "83b9fe94a405"
MGMKEY = "e1a055640fd1b0ef5eb5b9dfd5c867d3e035bf2885ca5276"


def main():
    print("\nMFA4 SciLAN/HPC Programmer\n")

    if len(sys.argv) == 2 and sys.argv[1] == "-c":
        create_key_pair()
    else:
        program_user_key()


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


def pivTool(args: str) -> Tuple[bytes, bytes]:
    return run(yubicoPivTool, args)


def ykPers(args: str) -> Tuple[bytes, bytes]:
    return run(ykpersonalize, args)


def ykChalResp(args: str):
    return run(ykchalresp, args)


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
    except ValueError:
        print("User Certificate verification failure.  Exiting.")
        sys.exit()
    print("User Certificate for slot " + slot + " created and verified.")

    print("Pushing user certificate to yubikey, slot " + slot + ".")
    userCertPem = crypto.dump_certificate(crypto.FILETYPE_PEM, userCert)
    result = subprocess.run(
        [yubicoPivTool, "-k", mgmKey, "-s", slot, "-a", "import-certificate"],
        input=userCertPem,
        check=True,
    )
    print(result.stdout)

    result = subprocess.run(
        [yubicoPivTool, "-s", slot, "-a", "read-certificate"],
        check=True,
    )
    userCertPem2 = result.stdout

    if userCertPem != userCertPem2:
        print("User Certificate on key not verified.  Exiting.")
        sys.exit()
    print("User Certificate on key is verified.")

    print("Pushing user private key to yubikey, slot " + slot + ".")
    result = subprocess.run(
        [yubicoPivTool, "-k" + mgmKey, "-s", slot, "-a", "import-key"],
        input=crypto.dump_privatekey(crypto.FILETYPE_PEM, userKeyPair),
        check=True,
    )
    print(result.stdout)

    print("Verifying RSA2048 signature with key in slot " + slot + ".")
    result = subprocess.run(
        [
            yubicoPivTool,
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
        check=True,
    )
    print(result.stdout)


def create_key_pair():
    """Handle Key Pair Creation"""
    print("Preparing to create a new Certificate Authority key pair.")
    print("This will overwrite any existing keys in " + storagePath + ".")
    yOrN = input("Do you want to continue? ")
    if yOrN.lower() not in ["y", "yes"]:
        print("\nNo file changes made.  Exiting.\n")
        return
    print("\nCreating RSA4096 key pair for Certificate Authority.")
    CAKeyPair = crypto.PKey()
    CAKeyPair.generate_key(crypto.TYPE_RSA, 4096)

    while True:
        privatePass = getpass.getpass("\nEnter passphrase for CA Private Key: ")
        privatePass2 = getpass.getpass("Re-enter passphrase: ")
        if privatePass == privatePass2:
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
    except ValueError:
        print("CA Certificate verification failure.  Exiting.")
        sys.exit()
    print("CA Certificate verification completed.")

    print("\nWriting CA Certificate to " + storagePath + "CACertificate.pem.")
    with open(storagePath + "CACertificate.pem", "wb") as CACertFile:
        CACertFile.write(crypto.dump_certificate(crypto.FILETYPE_PEM, CACert))

    print("Writing Encrypted CA Private Key to " + storagePath + "CAPrivateKey.pem.")
    with open(storagePath + "CAPrivateKey.pem", "wb") as CAPrivateKeyFile:
        CAPrivateKeyFile.write(
            crypto.dump_privatekey(
                crypto.FILETYPE_PEM,
                CAKeyPair,
                "aes-256-cbc",
                bytes(privatePass, "utf-8"),
            )
        )
    print("Exiting.\n")


def program_user_key():
    """User Key Creation and Yubikey Programming"""

    # Load and decrypt CA certificate and private key
    privatePass = getpass.getpass("Enter passphrase for CA Private Key: ")
    try:
        with open(storagePath + "CAPrivateKey.pem", "rb") as CAPrivateKeyFile:
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
        with open(storagePath + "CACertificate.pem", "rb") as CACertFile:
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
            serialText = subprocess.check_output([ykinfo, "-sq"])
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
        chkResp = h.digest().hex()
        if keyResp.strip() != chkResp:
            print("Slot 2 HMAC test failure.  Exiting.")
            sys.exit()
        print("HMAC Slot 2 test complete.")

        # generate user's pin/puk
        keyPIN = base64.b64encode(sha256(str(serial) + str(pinGenerator))).decode("utf-8")[0:8]
        keyPUK = base64.b64encode(sha256(str(serial) + str(pukGenerator))).decode("utf-8")[0:8]
        print("\nSetting yubikey configuration data.")
        try:
            print(pivTool("-a set-chuid")[1])
            print(pivTool("-a set-ccc")[1])
            print(pivTool("-a set-mgm-key -n" + MGMKEY)[1])
            print(pivTool("-a change-pin -P 123456 -N" + keyPIN)[1])
            print(pivTool("-a change-puk -P 12345678 -N" + keyPUK)[1])
        except ValueError:
            sys.exit()
        print()

        # Program all three PIV certificate slots
        programRSASlot(MGMKEY, username, keyPIN, "9a", serial, CACert, CAPrivateKey)
        programRSASlot(MGMKEY, username, keyPIN, "9c", serial, CACert, CAPrivateKey)
        programRSASlot(MGMKEY, username, keyPIN, "9d", serial, CACert, CAPrivateKey)

        print("OTP Secret:  " + userOtp.hex())
        print("HMAC Secret: " + userHmac.hex())

        # Write out to log file
        with open(storagePath + "mfaKey.log", "a", encoding="utf-8") as f:
            out = "[" + time.asctime(time.localtime(time.time())) + "]   "
            out += username + "\t"
            out += str(serial) + "   "
            out += keyPIN + "\n"
            f.write(out)

        print("Key programmed and tested for user " + username + ".  Remove key.")


if __name__ == "__main__":
    main()
