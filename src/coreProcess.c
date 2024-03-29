#include "coreProcess.h"
#include "globalDefinitions.h"

u8 plainBuffer[sizeof(plain_t)];
plain_t *plain = (plain_t *)plainBuffer;

packet_t *packet = NULL;

void arrayToHexString(u8 *binary, char *str, int binLen)
{
    int i;
    memset(str, 0, binLen * 2 + 1);
    for (i = 0; i < binLen; i++)
        sprintf(str + 2 * i, "%02x", binary[i]);
    return;
}

/******************************************************/
/*    Core process that handles logging and crypto    */
/******************************************************/

void coreProcess(int coreToNetSocket, int coreToAuthSocket)
{
    char netBuffer[BUFFER_SIZE];
    FILE *lf, *cf, *pf;
    int serial, destPort, timeout, i, loginAddr = -1;
    char username[32], destSystem[addrSize];
    u8 remoteAddr[addrSize], serverAddr[MAX_NO_OF_SERVER_ADDR][addrSize] = {{0x00}};
    u8 connAddr[addrSize];
    u8 AES128Key[16], AES256Key[32], HMACKey[20];
    packet = (packet_t *)netBuffer;

    lf = fopen(logFile(), "a");
    if (lf == NULL)
        fatal("SERVER HALT - Error opening log file.");
    cf = fopen(counterFile(), "r+");
    if (cf == NULL)
        fatal("SERVER HALT - Error opening counter file.");
    pf = fopen(passwdFile(), "r");
    if (pf == NULL)
        fatal("SERVER HALT - Error opening password file.");

    /* Enter jail and drop privilege */
    {
        char processJailPath[MAX_PATH];
        strcpy(processJailPath, jailPath());
        strcat(processJailPath, "/core");
        mkdir(processJailPath, rwX);
        ABORT_IF_ERR(chown(processJailPath, 0, 0), "Core process could not chown jail path");
        ABORT_IF_ERR(chmod(processJailPath, rwX), "Core process could not chmod jail path");
        ABORT_IF_ERR(chdir(processJailPath), "Core process could not chdir jail path");
        ABORT_IF_ERR(chroot(processJailPath), "Core process could not chroot jail path");
        ABORT_IF_ERR(setgid(gid()), "Error in setgid()");
        ABORT_IF_ERR(setuid(uid()), "Error in setuid()");
        ABORT_IF_ERR((getgid() != gid()) || (getuid() != uid()),
                     "SERVER HALT - Privilege not dropped in Core process.");
    }

    if (noOfServerAddresses() == 0)
        fatal("SERVER HALT - No external addresses specified.");

    /* Dump initialization info into log file */
    logOutput(lf, 0, "Starting CAPD - Version %s", capdVersion());
    logOutput(lf, 0, "  Password File      : %s", passwdFile());
    logOutput(lf, 0, "  Counter File       : %s", counterFile());
    logOutput(lf, 0, "  Log File           : %s", logFile());
    logOutput(lf, 0, "  Jail Directory     : %s", jailPath());
    logOutput(lf, 0, "  Packet DeltaT      : %d", deltaT());
    logOutput(lf, 0, "  Login Timeout      : %d", initTimeout());
    logOutput(lf, 0, "  Spoof Timeout      : %d", spoofTimeout());
    logOutput(lf, 0, "  Firewall Script    : %s", openSSHPath());
    logOutput(lf, 0, "  Unprivileged User  : %s", user());
    logOutput(lf, 0, "  UDP Port           : %d", capPort());
    for (i = 0; i < noOfServerAddresses(); i++)
        logOutput(lf, 0, "  Interface Address %d: %s", i, serverAddress(i));
    logOutput(lf, 0, "  Verbosity Level    : %d", verbosity());
    logOutput(lf, 0, "  CAPD Drop Priv uid : %d", uid());
    logOutput(lf, 0, "  CAPD Drop Priv gid : %d", gid());

    /* Parse server addresses */
    for (i = 0; i < noOfServerAddresses(); i++)
    {
        struct sockaddr_in sa;
        inet_pton(AF_INET, serverAddress(i), &(sa.sin_addr));
        memcpy(serverAddr[i] + 12, &(sa.sin_addr), 4);
    }

    while (1)
    {
        /* Cryptographically parse and verify packet */
        if ((read(coreToNetSocket, remoteAddr, sizeof(remoteAddr)) != sizeof(remoteAddr)) ||
            (read(coreToNetSocket, netBuffer, BUFFER_SIZE) != sizeof(packet_t)))
        {
            logOutput(lf, 0, "CRITICAL FAULT - Wrong-sized message received from Net Process!");
            continue;
        }
        serial = packet->serial;

        /* Look up user information */
        if (!(lookUpSerial(pf, serial, username, destSystem, &destPort, AES128Key, HMACKey)))
        {
            logOutput(lf, 1, "Failed serial lookup: %d", serial);
            continue;
        }

        /* Construct SHA1-HMAC response and AES256 Key */
        {
            u8 response[20], tmp[100];
            SHA1HMAC(packet->challenge, 32, HMACKey, 20, response);
            memcpy(tmp, response, 20);
            memcpy(tmp + 20, packet->challenge, 32);
            SHA256Hash(tmp, 20 + 32, AES256Key);
        }

        /* Decrypt encBlock */
        decryptAES256CBC(packet->encBlock, sizeof(plain_t), AES256Key, packet->IV, plainBuffer);

        /* Verify plaintext chksum */
        {
            u8 digest[32];
            SHA256Hash(plainBuffer, chksumBlockSize, digest);
            if (memcmp((void *)digest, (void *)plain->chksum, chksumSize) != 0)
            {
                logOutput(lf, 1, "Decrypted block failed chksum.  Serial: %d", serial);
                continue;
            }
        }

        /* Verify addresses */
        {
            loginAddr = -1;
            for (i = 0; i < noOfServerAddresses(); i++)
                if (memcmp(plain->serverAddr, serverAddr[i], addrSize) == 0)
                {
                    loginAddr = i;
                    break;
                }
            if (loginAddr == -1)
            {
                char str[33];
                logOutput(lf, 1, "Bad server address in decrypted block.  Serial: %d", serial);
                arrayToHexString(plain->serverAddr, str, 16);
                logOutput(lf, 2, "     Decrypted Value: %s", str);
                continue;
            }
        }

        // Default (legacy) behavior initialize connAddr to connAddr in packet
        memcpy(connAddr, plain->connAddr, sizeof(plain->connAddr));
        if (memcmp(plain->authAddr, remoteAddr, addrSize) != 0)
        {
            char str[33];
            logOutput(lf, 1, "Bad client address in decrypted block.  Serial: %d", serial);
            arrayToHexString(remoteAddr, str, 16);
            logOutput(lf, 2, "     Packet Value   : %s", str);
            arrayToHexString(plain->authAddr, str, 16);
            logOutput(lf, 2, "     Decrypted Value: %s", str);
            if (!ignore_pkt_ip())
            {
                continue;
            }
            logOutput(lf, 2, "Ignoring Packet Mismatch...", str);
            //  Instead of connAddr in packet, set connAddr to address where packet came from
            memcpy(connAddr, remoteAddr, sizeof(remoteAddr));
        }

        /* Verify username */
        if (strcmp(plain->username, username) != 0)
        {
            logOutput(lf, 1, "Username verification failed: %s", plain->username);
            continue;
        }

        /* Verify challenge construction */
        {
            u8 tmp[100], digest[32];
            memcpy(tmp, "SHA1-HMACChallenge", 18);
            memcpy(tmp + 18, plain->OTP, 16);
            memcpy(tmp + 18 + 16, plain->entropy, 32);
            SHA256(tmp, 18 + 16 + 32, digest);
            if (memcmp(packet->challenge, digest, SHA256_DIGEST_LENGTH) != 0)
            {
                logOutput(lf, 1, "Challenge construction verification failed.  Serial: %d", serial);
                continue;
            }
        }

        /* Verify IV construction */
        {
            u8 tmp[100], digest[32];
            memcpy(tmp, plain->entropy, 32);
            memcpy(tmp + 32, plain->OTP, 16);
            SHA256Hash(tmp, 32 + 16, digest);
            if (memcmp(packet->IV, digest, 16) != 0)
            {
                logOutput(lf, 1, "IV construction verification failed.  Serial: %d", serial);
                continue;
            }
        }

        /* Verify OTP */
        {
            u8 OTPBuffer[sizeof(OTP_t)], tmp[100], digest[32];
            int counter;
            OTP_t *OTP = (OTP_t *)OTPBuffer;
            decryptAES128ECB(plain->OTP, AES128Key, OTPBuffer);

            /* Verify decrypted CRC */
            if (0xf0b8 != crc16(OTPBuffer, 16))
            {
                logOutput(lf, 1, "OTP CRC verification failed.  Serial: %d", serial);
                continue;
            }

            /* Check OTP Challenge */
            memcpy(tmp, "yubicoChal", 10);
            memcpy(tmp + 10, plain->entropy, 32);
            SHA256(tmp, 10 + 32, digest);
            if ((memcmp(digest, OTP->OTPChallenge, 6) != 0))
            {
                logOutput(lf, 1, "OTP Challenge verification failed.  Serial: %d", serial);
                continue;
            }

            counter = OTP->insertCounter * 256 + OTP->sessionCounter;
            /* Check counter */
            if (counter <= searchCounterFile(cf, serial))
            {
                logOutput(lf, 1, "OTP Counter verification failed.  Serial: %d", serial);
                continue;
            }
            updateCounterFileEntry(cf, serial, counter);
            logOutput(lf, 1, "Accepted connection: %s, %d, %d, %s", username, serial, counter,
                      serverAddress(loginAddr));
        }

        /* /\* Setup timeout *\/ */
        /* if (memcmp(plain->connAddr, plain->authAddr, addrSize) == 0) */
        timeout = initTimeout();
        /* else */
        /*     timeout = spoofTimeout(); */

        {
            /* Translate connIPbinary to IP as text */

            authmessage_t msg = {.destPort = destPort, .timeout = timeout, .loginAddrIndex = loginAddr};
            u8 allZeros[12] = {0x00};
            if (memcmp(connAddr, allZeros, 12) == 0) /* IPV4 */
            {
                u8 ipv4[4];
                memcpy(ipv4, connAddr + 12, 4);
                inet_ntop(AF_INET, ipv4, msg.connAddrTxt, INET_ADDRSTRLEN);
            }
            else /* IPV6 */
                inet_ntop(AF_INET6, connAddr, msg.connAddrTxt, INET6_ADDRSTRLEN);

            memcpy(msg.destSystem, destSystem, sizeof(destSystem));

            /* Authorize connection */
            ABORT_IF_ERR(write(coreToAuthSocket, &msg, sizeof(msg)), "Unable to send auth message to core process");
        }
    }
    return;
}
