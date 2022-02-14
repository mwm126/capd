#ifndef UTILITYFUNCTIONS_H
#define UTILITYFUNCTIONS_H

#include "errno.h"
#include "globalDefinitions.h"

/*********************************************/
/*             Utility Functions             */
/*********************************************/

void fatal(const char *msg);
void logOutput(FILE *f, int l, const char *fmt, ...);

/**********************************************************/
/*           Search CAPD Password File Function           */
/**********************************************************/
int lookUpSerial(FILE *f, int serial, char *username, char *destSystem, int *destPort, u8 *AES128Key, u8 *HMACKey);

/*********************************************/
/*           Endian swap functions           */
/*********************************************/
void identifyEndianess(void);
#endif
