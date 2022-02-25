#ifndef REPLAYFUNCTIONS_H
#define REPLAYFUNCTIONS_H

#include "globalDefinitions.h"

/***************************************/
/*       Replay Bitmap Functions       */
/***************************************/

#define MAPSIZE (4 * 1024 * 1024) /* Size of bitmap used for replay tests */
#define MAPSIZE_BITS (8 * MAPSIZE)

void bitSet(u32 in, u8 *A);
int bitTest(u32 in, u8 *A);
void mapClear(u8 *A);
void initMaps(void);
int replayTest(u32 key, int time);
void replaySet(u32 key, int time);
#endif
