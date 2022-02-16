#ifndef COREPROCESS_H
#define COREPROCESS_H

#include "counterFunctions.h"
#include "cryptoFunctions.h"
#include "globalDefinitions.h"
#include "utilityFunctions.h"

/******************************************************/
/*    Core process that handles logging and crypto    */
/******************************************************/

void coreProcess(int coreToNetSocket, int coreToAuthSocket);

#endif
