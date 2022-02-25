#ifndef CRYPTOFUNCTIONS_H
#define CRYPTOFUNCTIONS_H

#include "globalDefinitions.h"

/***********************************************/
/*     Cryptography Functions and Wrappers     */
/***********************************************/

void SHA256Hash(u8 *data, int dataLen, u8 *digest);
void SHA1HMAC(u8 *chal, int chalLen, u8 *key, int keyLen, u8 *resp);
void decryptAES128ECB(u8 *cryptText, u8 *key, u8 *plainText);
void decryptAES256CBC(u8 *cryptText, int cryptLen, u8 *key, u8 *IV, u8 *plainText);
u16 crc16(u8 *data, int dataLen);

#endif
