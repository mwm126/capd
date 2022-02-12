/***************************************************************/
/*  Cloaked Access Protocol Daemon                             */
/*  (C) 2012-2013 Aeolus Technologies, Inc.                    */
/*  All Rights Reserved.                                       */
/*                                                             */
/*  Build command: gcc -lrt -lcrypto -O3 -Wall -o capd capd.c  */
/***************************************************************/

#include "authProcess.h"
#include "coreProcess.h"
#include "counterFunctions.h"
#include "cryptoFunctions.h"
#include "globalDefinitions.h"
#include "netProcess.h"
#include "utilityFunctions.h"

int main(int argc, char *argv[])
{
    /* Test endianness */
    identifyEndianess();

    /* Parse commandline inputs */
    init_usage(argc, argv);

    /* Fork into processes and establish sockets connections */
    {
        int sockets[2];
        int netPID, corePID;
        int netToCoreSocket = 0, coreToNetSocket = 0;
        int coreToAuthSocket = 0, authToCoreSocket = 0;

        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        netPID = fork();
        if (!netPID)
        {
            netToCoreSocket = sockets[0];
            close(sockets[1]);
            netProcess(netToCoreSocket);
            return 0;
        }
        coreToNetSocket = sockets[1];
        close(sockets[0]);

        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        corePID = fork();
        if (!corePID)
        {
            coreToAuthSocket = sockets[0];
            close(sockets[1]);
            coreProcess(coreToNetSocket, coreToAuthSocket);
            return 0;
        }
        close(coreToNetSocket);
        coreToNetSocket = 0;
        authToCoreSocket = sockets[1];
        close(sockets[0]);

        authProcess(authToCoreSocket, netPID, corePID);
    }
    return 0;
}
