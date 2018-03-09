/*************************************************************/
/*   Opens UDP socket and does basic processing of packets   */
/*************************************************************/

#include "fixedKey.h"
#include "replayFunctions.h"
#include "utilityFunctions.h"
#include "cryptoFunctions.h"

void netProcess(int netToCoreSocket)
{
int udpSocket,len;
struct sockaddr_in siLocal, siRemote;
socklen_t lenRemote;
u8 udpBuffer[BUFFER_SIZE];
int localTime;
packet_t *packet=(packet_t *)udpBuffer;
u8 tmp[MACBlockSize+32+32];
u8 MAC[MACSize];
u32 *key=(u32 *)packet->MAC;

/* Clear out address structs */
memset(&siLocal,0,sizeof(siLocal));
memset(&siRemote,0,sizeof(siRemote));

/* De-obfuscate Shared Secrets */
deriveFixedKeys();

/* Setup Replay Bitmaps */
initMaps();

/* Open UDP Port */
udpSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
if (udpSocket < 0) fatal("SERVER HALT - UDP Socket creation failed.");

siLocal.sin_family=AF_INET;
siLocal.sin_port=htons(port);
siLocal.sin_addr.s_addr = htonl(INADDR_ANY);
if (bind(udpSocket,(struct sockaddr *)&siLocal,sizeof(siLocal)) < 0)
  fatal("SERVER HALT - Bind of UDP Socket failed.");

/* Enter jail and drop privilege */
{
char processJailPath[MAX_PATH];
strcpy(processJailPath,jailPath);
strcat(processJailPath,"/net");
mkdir(processJailPath,rwX);
chown(processJailPath,0,0);
chmod(processJailPath,rwX);
chdir(processJailPath);
chroot(processJailPath);
setgid(gid);
setuid(uid);
if ((getgid() != gid) || (getuid() != uid))
  fatal("SERVER HALT - Privilege not dropped in Net process.");
}

memset(udpBuffer,0,BUFFER_SIZE);
while(1)	/* Server loop */
  {
  lenRemote = sizeof(siRemote);
  len = recvfrom(udpSocket,udpBuffer,BUFFER_SIZE,0,
          (struct sockaddr *)&siRemote, &lenRemote);

  /* Basic tests */
  if (len != sizeof(packet_t)) continue;
  localTime=currentTime();
  if (abs(packet->timestamp-localTime)>deltaT) continue;

  /* replay test */
  if (replayTest(key[0],localTime)) continue;

  /* SHA256 MAC test */
  memcpy(tmp,fixedHeaderKey,32);
  memcpy(tmp+32,udpBuffer,MACBlockSize);
  memcpy(tmp+32+MACBlockSize,fixedFooterKey,32);
  SHA256Hash(tmp,32+MACBlockSize+32,MAC);
  if (memcmp(MAC,packet->MAC,MACSize) != 0) continue;

  /* Insert valid MAC into replay database */
  replaySet(key[0],localTime);

  /* Ship out packet to core process */
  {
  u8 remoteAddr[16] ={0x00};
  memcpy(remoteAddr+12,&(((struct sockaddr_in *)(&siRemote))->sin_addr),4);
  write(netToCoreSocket,remoteAddr,sizeof(remoteAddr));
  write(netToCoreSocket,udpBuffer,sizeof(packet_t));
  }
  }
return;
}
