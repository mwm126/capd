/*********************************************/
/*             Utility Functions             */
/*********************************************/
u32 currentTime(void)
{
struct timespec time1;
clock_gettime(CLOCK_REALTIME, &time1);
return (u32)time1.tv_sec;
}

void fatal(char *msg)
{
perror(msg);
exit(1);
}

void logOutput(FILE *f, int l, char *fmt, ...)
{
char timeStr[100];
time_t rawtime;
struct tm *info;
va_list argp;
if (verbosity < l) return;
time (&rawtime);
info = localtime(&rawtime);
strftime(timeStr,100,"%X %x",info);
fprintf(f,"[%s] ",timeStr);
va_start(argp,fmt);
vfprintf(f,fmt,argp);
va_end(argp);
fprintf(f,"\n");
fflush(f);
return;
}

void arrayToHexString ( u8 *binary, char *str, int binLen)
{
int i;
memset(str,0,binLen*2+1);
for (i=0;i<binLen;i++)
	sprintf(str+2*i,"%02x",binary[i]);
return;
}

/**********************************************************/
/*           Search CAPD Password File Function           */
/**********************************************************/
int lookUpSerial (	FILE *f, int serial, char *username, char *destSystem, 
					int *destPort, u8 *AES128Key, u8 *HMACKey)
{
int i,s;
char txtPort[8+1],txtS[20+1],txtAES[40+1],txtHMAC[50+1];
rewind(f);
while(fscanf(f,"%32s %32s %8s %20s %40s %50s\n",
		username,destSystem,txtPort,txtS,txtAES,txtHMAC) != EOF)
	{
	if (username[0] != 35)
		{
		s = atoi(txtS);
		if (serial == s)
			{
			int data;
			*destPort = atoi(txtPort);
			for (i=0;i<16;i++)
				{
				sscanf((const char *)(txtAES+2*i),"%2x",&data);
				AES128Key[i]=(u8)data;
				}
			for (i=0;i<20;i++)
				{
				sscanf((const char *)(txtHMAC+2*i),"%2x",&data);
				HMACKey[i]=(u8)data;
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
int endian=0;
enum{LITTLE,BIG};

void identifyEndianess(void)
{
u32 test32=0x00112233;
u8 *test8=(u8 *)&test32;
if (test8[0]) endian=LITTLE;
else endian=BIG;
return;
}

u16 swap16(u16 input)
{
u16 in16,out16;
u8 *in8 = (u8 *)&in16, *out8 = (u8 *)&out16;
if (endian==LITTLE) return input;
in16 = input;
out8[0] = in8[1];
out8[1] = in8[0];
return out16;
}

u32 swap32(u32 input)
{
u32 in32,out32;
u8 *in8 = (u8 *)&in32, *out8 = (u8 *)&out32;
in32 = input;
if (endian==LITTLE) return input;
out8[0] = in8[3];
out8[1] = in8[2];
out8[2] = in8[1];
out8[3] = in8[0];
return out32;
}

u64 swap64(u64 input)
{
u64 in64,out64;
u8 *in8 = (u8 *)&in64, *out8 = (u8 *)&out64;
in64 = input;
if (endian==LITTLE) return input;
out8[0] = in8[7];
out8[1] = in8[6];
out8[2] = in8[5];
out8[3] = in8[4];
out8[4] = in8[3];
out8[5] = in8[2];
out8[6] = in8[1];
out8[7] = in8[0];
return out64;
}