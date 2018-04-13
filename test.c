#include "cryptoFunctions.h"
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
  decryptAES128ECB(plain->OTP, AES128Key, OTPBuffer);
}
