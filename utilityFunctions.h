#ifndef UTILITYFUNCTIONS_H
#define UTILITYFUNCTIONS_H

#include "errno.h"
#include "globalDefinitions.h"

/*********************************************/
/*             Utility Functions             */
/*********************************************/

void fatal(char *msg);
void logOutput(FILE *f, int l, char *fmt, ...);

/**********************************************************/
/*           Search CAPD Password File Function           */
/**********************************************************/
int lookUpSerial(FILE *f, int serial, char *username, char *destSystem, int *destPort, u8 *AES128Key, u8 *HMACKey);

/*********************************************/
/*           Endian swap functions           */
/*********************************************/
void identifyEndianess(void);
u16 swap16(u16 input);
u32 swap32(u32 input);
u64 swap64(u64 input);
#endif
