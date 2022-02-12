#ifndef NETPROCESS_H
#define NETPROCESS_H

/*************************************************************/
/*   Opens UDP socket and does basic processing of packets   */
/*************************************************************/

#include "cryptoFunctions.h"
#include "globalDefinitions.h"
#include "replayFunctions.h"
#include "utilityFunctions.h"

void netProcess(int netToCoreSocket);

#endif
