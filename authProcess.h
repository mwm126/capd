#ifndef AUTHPROCESS_H
#define AUTHPROCESS_H

#include "globalDefinitions.h"
#include "utilityFunctions.h"

/************************************/
/*   Authorization of connections   */
/************************************/

#define MAX_CMDLINE (MAX_PATH + 150)

void build_command_line(char commandLine[], char allowedAddr[addrTxtSize], char hostAddr[32], int timeLimit,
                        char destAddr[32], int destPort);

void authProcess(int authToCoreSocket, int netPID, int corePID);
#endif
