#ifndef REPLAYFUNCTIONS_H
#define REPLAYFUNCTIONS_H

/***************************************/
/*       Replay Bitmap Functions       */
/***************************************/

#define MAPSIZE (4 * 1024 * 1024) /* Size of bitmap used for replay tests */
#define MAPSIZE_BITS (8 * MAPSIZE)

u8 *map[3] = {NULL};
int mapWindow[3] = {0, 0, 0};

void bitSet(u32 in, u8 *A)
{
    int i, j, k;
    i = in % MAPSIZE_BITS;
    j = i >> 3;
    k = i - (j << 3);
    A[j] |= (1 << k);
    return;
}

int bitTest(u32 in, u8 *A)
{
    int i, j, k;
    i = in % MAPSIZE_BITS;
    j = i >> 3;
    k = i - (j << 3);
    return (A[j] & (1 << k));
}

void mapClear(u8 *A)
{
    memset(A, 0, MAPSIZE);
    return;
}

void initMaps(void)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        if (map[i] == NULL)
            map[i] = (u8 *)malloc(MAPSIZE);
        mapClear(map[i]);
    }
    return;
}

int replayTest(u32 key, int time)
{
    int i, modKey = key % MAPSIZE_BITS, window = time / deltaT;
    for (i = 0; i < 3; i++)
    {
        if (mapWindow[i] == window - 1)
            if (bitTest(modKey, map[i]))
                return 1;
        if (mapWindow[i] == window)
            if (bitTest(modKey, map[i]))
                return 1;
        if (mapWindow[i] == window + 1)
            if (bitTest(modKey, map[i]))
                return 1;
    }
    return 0;
}

void replaySet(u32 key, int time)
{
    int i, modKey = key % MAPSIZE_BITS, window = time / deltaT;
    for (i = 0; i < 3; i++)
        if (mapWindow[i] == window)
        {
            bitSet(modKey, map[i]);
            return;
        }
    for (i = 0; i < 3; i++)
        if ((mapWindow[i] != window - 1) && (mapWindow[i] != window) && (mapWindow[i] != window + 1))
        {
            mapWindow[i] = window;
            mapClear(map[i]);
            bitSet(modKey, map[i]);
            return;
        }
    return;
}
#endif
