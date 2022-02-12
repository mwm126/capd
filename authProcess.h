#ifndef AUTHPROCESS_H
#define AUTHPROCESS_H

#include "globalDefinitions.h"
#include "utilityFunctions.h"

/************************************/
/*   Authorization of connections   */
/************************************/

#define MAX_CMDLINE (MAX_PATH + 150)
void build_command_line(char commandLine[], char allowedAddr[addrTxtSize], char hostAddr[32], int timeLimit);

void authProcess(int authToCoreSocket, int netPID, int corePID)
{
    char commandLine[MAX_PATH + 150];
    char hostAddr[32];
    char allowedAddr[addrTxtSize];
    int port, timeLimit;

    /* This process retains privilege and is unjailed. */
    while (1)
    {
        /* Wait for authorization information */
        if ((read(authToCoreSocket, allowedAddr, sizeof(allowedAddr)) != sizeof(allowedAddr)) ||
            (read(authToCoreSocket, hostAddr, sizeof(hostAddr)) != sizeof(hostAddr)) ||
            (read(authToCoreSocket, &port, sizeof(port)) != sizeof(port)) ||
            (read(authToCoreSocket, &timeLimit, sizeof(timeLimit)) != sizeof(timeLimit)))
        /* Kill all capd processes if wrong-sized message arrives. */
        {
            kill(netPID, SIGKILL);
            kill(corePID, SIGKILL);
            fatal("CRITICAL FAULT - Wrong-sized message received from Auth Process!");
        }

        /* Sanitize allowedAddr and hostAddr to prevent script hijacking */
        for (size_t i = 0; i < sizeof(allowedAddr); i++)
            if (!((allowedAddr[i] == 0) ||                               /* Null OK */
                  (allowedAddr[i] == 46) ||                              /* Period OK */
                  (allowedAddr[i] == 58) ||                              /* Colon OK (IPV6) */
                  ((allowedAddr[i] >= 48) && (allowedAddr[i] <= 57)) ||  /* 0-9 OK */
                  ((allowedAddr[i] >= 97) && (allowedAddr[i] <= 102)) || /* a-f OK (IPV6) */
                  ((allowedAddr[i] >= 65) && (allowedAddr[i] <= 70))     /* A-F OK (IPV6) */
                  ))
                allowedAddr[i] = 46; /* Replace bad character with period */
        for (size_t i = 0; i < sizeof(hostAddr); i++)
            if (!((hostAddr[i] == 0) ||                            /* Null OK */
                  (hostAddr[i] == 46) ||                           /* Period OK */
                  (hostAddr[i] == 58) ||                           /* Colon OK (IPV6) */
                  (hostAddr[i] == 45) || (hostAddr[i] == 95) ||    /* hyphen and underscore */
                  ((hostAddr[i] >= 48) && (hostAddr[i] <= 57)) ||  /* 0-9 OK */
                  ((hostAddr[i] >= 97) && (hostAddr[i] <= 122)) || /* a-z OK */
                  ((hostAddr[i] >= 65) && (hostAddr[i] <= 90))     /* A-Z OK */
                  ))
                hostAddr[i] = 46; /* Replace bad character with period */

        /* Place hard limits on int values */
        timeLimit = BOUND(timeLimit, 0, 300);
        port = BOUND(port, 0, 65535);

        build_command_line(commandLine, allowedAddr, hostAddr, timeLimit);

        /* Execute openSSH.sh command */
        system(commandLine);
        //	printf("%s\n",commandLine);
    }
    return;
}

void build_command_line(char commandLine[], char allowedAddr[addrTxtSize], char hostAddr[32], int timeLimit)
{
    char tmp[15];
    /* Build command line for openSSH.sh call */
    memset(commandLine, 0, MAX_CMDLINE);
    strcpy(commandLine, openSSHPath);
    strcat(commandLine, " ");
    strcat(commandLine, allowedAddr);
    strcat(commandLine, " ");
    strcat(commandLine, hostAddr);
    strcat(commandLine, " ");
    sprintf(tmp, "%d", port);
    strcat(commandLine, tmp);
    strcat(commandLine, " ");
    sprintf(tmp, "%d", timeLimit);
    strcat(commandLine, tmp);
    strcat(commandLine, " &");
}

#endif
