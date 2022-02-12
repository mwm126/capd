/*************************************************************/
/*   Opens UDP socket and does basic processing of packets   */
/*************************************************************/

#include "netProcess.h"

/************************************************************/
/*    Derives shared keys from seed and simple algorithm    */
/************************************************************/

u8 seedHeader[32] = {0xef, 0xa8, 0xfe, 0x36, 0xeb, 0x80, 0x02, 0x5c, 0x0f, 0xfd, 0x09, 0x1a, 0xa9, 0x1c, 0x50, 0xf8,
                     0x3e, 0xeb, 0x52, 0x74, 0x9c, 0x56, 0xa4, 0x44, 0x7b, 0x31, 0x6c, 0x1a, 0xe5, 0xbc, 0xf7, 0x5d};
u8 seedFooter[32] = {0x42, 0x3c, 0x05, 0xf2, 0xc4, 0x9b, 0x8c, 0x3e, 0x79, 0x16, 0xba, 0xd2, 0x54, 0xd7, 0x92, 0x48,
                     0xc2, 0x55, 0xba, 0x8c, 0xe2, 0xe5, 0xe5, 0xd2, 0x1b, 0x1a, 0x1c, 0xbc, 0x49, 0xab, 0x28, 0x18};
u8 fixedHeaderKey[32];
u8 fixedFooterKey[32];

void deriveFixedKeys(void)
{
    int i, generator = 0x55;
    for (i = 0; i < 32; i++)
    {
        int byte = seedHeader[i];
        generator = (generator * byte + 0x12) % 256;
        fixedHeaderKey[i] = generator ^ byte;
    }
    generator = 0x18;
    for (i = 0; i < 32; i++)
    {
        int byte = seedFooter[i];
        generator = (generator * byte + 0xe2) % 256;
        fixedFooterKey[i] = generator ^ byte;
    }
    for (i = 0; i < 32; i++)
        seedHeader[i] = 0xff;
    for (i = 0; i < 32; i++)
        seedFooter[i] = 0xff;
    return;
}

u32 currentTime(void)
{
    struct timespec time1;
    clock_gettime(CLOCK_REALTIME, &time1);
    return (u32)time1.tv_sec;
}

void netProcess(int netToCoreSocket)
{
    int udpSocket, len;
    struct sockaddr_in siLocal, siRemote;
    socklen_t lenRemote;
    u8 udpBuffer[BUFFER_SIZE];
    int localTime;
    packet_t *packet = (packet_t *)udpBuffer;
    u8 tmp[MACBlockSize + 32 + 32];
    u8 MAC[MACSize];
    u32 *key = (u32 *)packet->MAC;

    /* Clear out address structs */
    memset(&siLocal, 0, sizeof(siLocal));
    memset(&siRemote, 0, sizeof(siRemote));

    /* De-obfuscate Shared Secrets */
    deriveFixedKeys();

    /* Setup Replay Bitmaps */
    initMaps();

    /* Open UDP Port */
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket < 0)
        fatal("SERVER HALT - UDP Socket creation failed.");

    siLocal.sin_family = AF_INET;
    siLocal.sin_port = htons(port());
    siLocal.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(udpSocket, (struct sockaddr *)&siLocal, sizeof(siLocal)) < 0)
        fatal("SERVER HALT - Bind of UDP Socket failed.");

    /* Enter jail and drop privilege */
    {
        char processJailPath[MAX_PATH];
        strcpy(processJailPath, jailPath());
        strcat(processJailPath, "/net");
        mkdir(processJailPath, rwX);
        chown(processJailPath, 0, 0);
        chmod(processJailPath, rwX);
        chdir(processJailPath);
        chroot(processJailPath);
        setgid(gid());
        setuid(uid());
        if ((getgid() != gid()) || (getuid() != uid()))
            fatal("SERVER HALT - Privilege not dropped in Net process.");
    }

    memset(udpBuffer, 0, BUFFER_SIZE);
    while (1) /* Server loop */
    {
        lenRemote = sizeof(siRemote);
        len = recvfrom(udpSocket, udpBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&siRemote, &lenRemote);

        /* Basic tests */
        if (len != sizeof(packet_t))
            continue;
        localTime = currentTime();
        if (abs(packet->timestamp - localTime) > (int)deltaT())
            continue;

        /* replay test */
        if (replayTest(key[0], localTime))
            continue;

        /* SHA256 MAC test */
        memcpy(tmp, fixedHeaderKey, 32);
        memcpy(tmp + 32, udpBuffer, MACBlockSize);
        memcpy(tmp + 32 + MACBlockSize, fixedFooterKey, 32);
        SHA256Hash(tmp, 32 + MACBlockSize + 32, MAC);
        if (memcmp(MAC, packet->MAC, MACSize) != 0)
            continue;

        /* Insert valid MAC into replay database */
        replaySet(key[0], localTime);

        /* Ship out packet to core process */
        {
            u8 remoteAddr[16] = {0x00};
            memcpy(remoteAddr + 12, &(((struct sockaddr_in *)(&siRemote))->sin_addr), 4);
            write(netToCoreSocket, remoteAddr, sizeof(remoteAddr));
            write(netToCoreSocket, udpBuffer, sizeof(packet_t));
        }
    }
    return;
}
