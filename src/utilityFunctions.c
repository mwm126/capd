#include "errno.h"
#include "globalDefinitions.h"

#include "utilityFunctions.h"

/*********************************************/
/*             Utility Functions             */
/*********************************************/

void fatal(const char *msg)
{
    if (errno)
    {
        perror(msg);
    }
    else
    {
        printf("%s\n", msg);
    }
    exit(1);
}

void logOutput(FILE *f, int l, const char *fmt, ...)
{
    char timeStr[100];
    time_t rawtime;
    struct tm *info;
    va_list argp;
    if (verbosity() < l)
        return;
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(timeStr, 100, "%X %x", info);
    fprintf(f, "[%s] ", timeStr);
    va_start(argp, fmt);
    vfprintf(f, fmt, argp);
    va_end(argp);
    fprintf(f, "\n");
    fflush(f);
    return;
}

/**********************************************************/
/*           Search CAPD Password File Function           */
/**********************************************************/
int lookUpSerial(FILE *f, int serial, char *username, char *destSystem, int *destPort, u8 *AES128Key, u8 *HMACKey)
{
    int i, s;
    char txtPort[8 + 1], txtS[20 + 1], txtAES[40 + 1], txtHMAC[50 + 1];
    rewind(f);
    while (fscanf(f, "%32s %32s %8s %20s %40s %50s\n", username, destSystem, txtPort, txtS, txtAES, txtHMAC) != EOF)
    {
        if (username[0] != 35)
        {
            s = atoi(txtS);
            if (serial == s)
            {
                unsigned int data;
                *destPort = atoi(txtPort);
                for (i = 0; i < 16; i++)
                {
                    sscanf((const char *)(txtAES + 2 * i), "%2x", &data);
                    AES128Key[i] = (u8)data;
                }
                for (i = 0; i < 20; i++)
                {
                    sscanf((const char *)(txtHMAC + 2 * i), "%2x", &data);
                    HMACKey[i] = (u8)data;
                }
                return 1;
            }
        }
    }
    return 0;
}

/*********************************************/
/*           Endian swap functions           */
/*********************************************/
int endian = 0;
enum
{
    LITTLE,
    BIG
};

void identifyEndianess(void)
{
    u32 test32 = 0x00112233;
    u8 *test8 = (u8 *)&test32;
    if (test8[0])
        endian = LITTLE;
    else
        endian = BIG;
    return;
}
