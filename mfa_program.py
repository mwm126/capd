#!/usr/bin/env python
from OpenSSL import crypto, rand
import subprocess, getpass, sys, hashlib, hmac, time

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
hmacGenerator = hmacGenTxt.decode("base64")
otpGenerator = otpGenTxt.decode("base64")
pinGenTxt = "bseTuIXxT5E+Z7zKZlHnRzmv8J5/UY315O08z+N/LL7Z1LRku9ocQJbKjYgNMURz"
pinGenerator = pinGenTxt.decode("base64")
pukGenTxt = "/7jZypeK4aQqBFZvecVrgRep+ZBIEiu4TQP4lQtamYbdV0UJgvaEMsTYOG4bylX4"
pukGenerator = pukGenTxt.decode("base64")
mfaAccess = "83b9fe94a405"
mgmKey = "e1a055640fd1b0ef5eb5b9dfd5c867d3e035bf2885ca5276"


def sha256(string):
    m = hashlib.sha256()
    m.update(string)
    return m.digest()


def sha1(string):
    m = hashlib.sha1()
    m.update(string)
    return m.digest()


def pivTool(args):
    argList = [yubicoPivTool] + args.split()
    p = subprocess.Popen(argList, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return out, err


def ykPers(args):
    argList = [ykpersonalize] + args.split()
    #  subprocess.call(argList)
    p = subprocess.Popen(argList, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return out, err


def ykChalResp(args):
    argList = [ykchalresp] + args.split()
    #  subprocess.call(argList)
    p = subprocess.Popen(argList, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return out, err


def programRSASlot(mgmKey, username, keyPIN, slot):
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
    except:
        print("User Certificate verification failure.  Exiting.")
        exit()
    print("User Certificate for slot " + slot + " created and verified.")

    print("Pushing user certificate to yubikey, slot " + slot + ".")
    userCertPem = crypto.dump_certificate(crypto.FILETYPE_PEM, userCert)
    p = subprocess.Popen(
        [yubicoPivTool, "-k" + mgmKey, "-s", slot, "-a", "import-certificate"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )
    p.stdin.write(userCertPem)
    p.stdin.close()
    print(p.stdout.read())

    p = subprocess.Popen(
        [yubicoPivTool, "-s", slot, "-a", "read-certificate"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )
    userCertPem2 = p.stdout.read()

    if userCertPem != userCertPem2:
        print("User Certificate on key not verified.  Exiting.")
        exit()
    else:
        print("User Certificate on key is verified.")

    print("Pushing user private key to yubikey, slot " + slot + ".")
    p = subprocess.Popen(
        [yubicoPivTool, "-k" + mgmKey, "-s", slot, "-a", "import-key"],
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )
    p.stdin.write(crypto.dump_privatekey(crypto.FILETYPE_PEM, userKeyPair))
    p.stdin.close()
    print(p.stdout.read())

    print("Verifying RSA2048 signature with key in slot " + slot + ".")
    p = subprocess.Popen(
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
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )
    p.stdin.write(userCertPem2)
    p.stdin.close()
    print(p.stdout.read())


print("\nMFA4 SciLAN/HPC Programmer\n")

# Handle Key Pair Creation
if len(sys.argv) == 2 and sys.argv[1] == "-c":
    print("Preparing to create a new Certificate Authority key pair.")
    print("This will overwrite any existing keys in " + storagePath + ".")
    yOrN = input("Do you want to continue? ")
    if yOrN == "y" or yOrN == "Y" or yOrN == "yes" or yOrN == "Yes":

        print("\nCreating RSA4096 key pair for Certificate Authority.")
        CAKeyPair = crypto.PKey()
        CAKeyPair.generate_key(crypto.TYPE_RSA, 4096)

        while True:
            privatePass = getpass.getpass("\nEnter passphrase for CA Private Key: ")
            privatePass2 = getpass.getpass("Re-enter passphrase: ")
            if privatePass != privatePass2:
                print("Passphases do not match.")
                continue
            else:
                break

        print("\nCreating CA Certificate.")
        CACertFile = open(storagePath + "CACertificate.pem", "w")
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
        except:
            print("CA Certificate verification failure.  Exiting.")
            exit()
        print("CA Certificate verification completed.")

        print("\nWriting CA Certificate to " + storagePath + "CACertificate.pem.")
        CACertFile.write(crypto.dump_certificate(crypto.FILETYPE_PEM, CACert))
        CACertFile.close()

        print(
            "Writing Encrypted CA Private Key to " + storagePath + "CAPrivateKey.pem."
        )
        CAPrivateKeyFile = open(storagePath + "CAPrivateKey.pem", "w")
        CAPrivateKeyFile.write(
            crypto.dump_privatekey(
                crypto.FILETYPE_PEM, CAKeyPair, "aes-256-cbc", privatePass
            )
        )
        CAPrivateKeyFile.close()

        print("Exiting.\n")

    else:
        print("\nNo file changes made.  Exiting.\n")
    exit()

# Handle User Key Creation and Yubikey Programming

# Load and decrypt CA certificate and private key
privatePass = getpass.getpass("Enter passphrase for CA Private Key: ")
try:
    CAPrivateKeyFile = open(storagePath + "CAPrivateKey.pem", "r")
    CAPrivateKey = crypto.load_privatekey(
        crypto.FILETYPE_PEM, CAPrivateKeyFile.read(), privatePass
    )
    CAPrivateKeyFile.close()
except:
    print("Error opening CA Key.")
    print("Either the key file cannot be found or passphrase is incorrect.")
    print("Run with -c option to create a new CA key pair.")
    print("Exiting.")
    exit()
try:
    CACertFile = open(storagePath + "CACertificate.pem", "r")
    CACert = crypto.load_certificate(crypto.FILETYPE_PEM, CACertFile.read())
    CACertFile.close()
except:
    print("Error opening CA Certificate.")
    print("Run with -c option to create a new CA key pair.")
    print("Exiting.")
    exit()
print("CA Key successfully unlocked.")

# Forever loop for user creation without re-entering passphrase
while True:

    # prompt for new username/exit
    print("\nInsert new Yubikey 4 and enter username or press enter to exit: ")
    username = input("Username: ")
    if username == "":
        print("Exiting.")
        exit()

    # get serial number from yubikey
    try:
        serialText = subprocess.check_output([ykinfo, "-sq"])
        serial = int(serialText)
    except:
        print("Problem detecting Yubikey.")
        continue
    if serial < 4000000:
        print("Yubikey inserted is not a Yubikey 4")
        exit()
    print("Found Yubikey 4 with serial number: ", serial)

    # reset the key
    print("\nResetting yubikey.")

    ykPers("-y -z -1 -c" + mfaAccess)
    ykPers("-y -z -2 -c" + mfaAccess)

    pivTool("-a verify-pin -P 1001")
    pivTool("-a verify-pin -P 1001")
    pivTool("-a verify-pin -P 1001")
    pivTool("-a verify-pin -P 1001")
    pivTool("-a verify-pin -P 1002")
    pivTool("-a verify-pin -P 1002")
    pivTool("-a verify-pin -P 1002")
    pivTool("-a verify-pin -P 1002")

    pivTool("-a change-puk -P 1003 -N 1000001")
    pivTool("-a change-puk -P 1003 -N 1000001")
    pivTool("-a change-puk -P 1003 -N 1000001")
    pivTool("-a change-puk -P 1003 -N 1000001")
    pivTool("-a change-puk -P 1003 -N 1000002")
    pivTool("-a change-puk -P 1003 -N 1000002")
    pivTool("-a change-puk -P 1003 -N 1000002")
    pivTool("-a change-puk -P 1003 -N 1000002")
    pivTool("-a reset")

    # generate user's secrets
    hashID = sha256(username + str(serial) + hashIDSalt)
    userHmac = sha1(hashID + hmacGenerator)
    userOtp = sha1(hashID + otpGenerator)[0:16]

    # program the keys with hmac secrets
    print("\nProgramming OTP secret in Slot 1.")
    command = "-y -1 -c" + mfaAccess + " -ochal-resp -ochal-yubico "
    command += " -a" + userOtp.encode("hex") + " -oaccess=" + mfaAccess
    command += " -oserial-usb-visible -oserial-api-visible"
    ykPers(command)
    chal = rand.bytes(63)
    keyResp, err = ykChalResp("-1 -Y -x " + chal.encode("hex"))

    #  h = hmac.new(userOtp,chal,hashlib.sha1)
    #  chkResp = h.digest().encode('hex')
    #  if keyResp.strip() != chkResp:
    #    print "Slot 1 OTP test failure.  Exiting."
    #    exit()
    #  print "OTP Slot 1 test complete."

    print("\nProgramming HMAC secret in Slot 2.")
    command = "-y -2 -c" + mfaAccess + " -ochal-resp -ochal-hmac -ohmac-lt64"
    command += " -a" + userHmac.encode("hex") + " -oaccess=" + mfaAccess
    command += " -oserial-usb-visible -oserial-api-visible"
    ykPers(command)
    chal = rand.bytes(63)
    keyResp, err = ykChalResp("-2 -H -x " + chal.encode("hex"))
    h = hmac.new(userHmac, chal, hashlib.sha1)
    chkResp = h.digest().encode("hex")
    if keyResp.strip() != chkResp:
        print("Slot 2 HMAC test failure.  Exiting.")
        exit()
    print("HMAC Slot 2 test complete.")

    # generate user's pin/puk
    keyPIN = sha256(str(serial) + pinGenerator).encode("base64")[0:8]
    keyPUK = sha256(str(serial) + pukGenerator).encode("base64")[0:8]
    print("\nSetting yubikey configuration data.")
    try:
        sys.stdout.write(pivTool("-a set-chuid")[1])
        sys.stdout.write(pivTool("-a set-ccc")[1])
        sys.stdout.write(pivTool("-a set-mgm-key -n" + mgmKey)[1])
        sys.stdout.write(pivTool("-a change-pin -P 123456 -N" + keyPIN)[1])
        sys.stdout.write(pivTool("-a change-puk -P 12345678 -N" + keyPUK)[1])
    except:
        exit()
    print()

    # Program all three PIV certificate slots
    programRSASlot(mgmKey, username, keyPIN, "9a")
    programRSASlot(mgmKey, username, keyPIN, "9c")
    programRSASlot(mgmKey, username, keyPIN, "9d")

    print("OTP Secret:  " + userOtp.encode("hex"))
    print("HMAC Secret: " + userHmac.encode("hex"))

    # Write out to log file
    f = open(storagePath + "mfaKey.log", "a")
    out = "[" + time.asctime(time.localtime(time.time())) + "]   "
    out += username + "\t"
    out += str(serial) + "   "
    out += keyPIN + "\n"
    f.write(out)
    f.close()

    print("Key programmed and tested for user " + username + ".  Remove key.")
