#include "cryptoFunctions.h"
#include "utilityFunctions.h"
#include "stdio.h"

int main(int argc, char *argv[]) {
  FILE *lf, *cf, *pf;
  int serial, destPort, timeout;
  char username[32], destSystem[32];
  u8 remoteAddr[addrSize], serverAddr[addrSize] = {0x00};
  u8 AES128Key[16], AES256Key[32], HMACKey[20];
  pf = fopen("/home/mark/repos/capd/my.passwd", "r");
  if (!(lookUpSerial(pf, serial, username, destSystem, &destPort, AES128Key,
                     HMACKey))) {
    printf("FAILED.\n");
    return;
  }

  char ykresp_mod[32] = "cgddvdcdruclubdbgvfgevrhfrfvbrun";
  char ykresp_hex[32+1]; ykresp_hex[32] = 0;
  char modHexList[] = "cbdefghijklnrtuv";
  char hexList[]    = "0123456789abcdef";
  for (int i=0; i<32; i++) {
    for (int j=0; j<16; j++) {
      if (ykresp_mod[i]==modHexList[j]) {
        ykresp_hex[i] = hexList[j];
        break;
      }
    }
  }
  printf("decoded ykresp_hex: %s\n", ykresp_hex);

  char ykresp[16];
  for (int i=0; i<16; i++) {
    sscanf(ykresp_hex + 2*i, "%02x", &ykresp[i]);
    /* ykresp[i] = ykresp_hex[i+1]*256 + ykresp_hex[i]; */
  }

  u8 OTPBuffer[sizeof(OTP_t)], tmp[100], digest[32];

  printf("going to decrypt with key: "); phex(AES128Key, 16);
  /* printf("going to decrypt:  "); phex(plain->OTP, 16); */
  printf("going to decrypt ykresp : "); phex(ykresp, 16);
  decryptAES128ECB(ykresp, AES128Key, OTPBuffer);
  printf("got decrypted:  "); phex(OTPBuffer, sizeof(OTP_t));

  if (0xf0b8 != crc16(OTPBuffer, 16)) {
    printf("OTP CRC verification failed.\n");
  }

}
