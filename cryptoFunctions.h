#ifndef CRYPTOFUNCTIONS_H
#define CRYPTOFUNCTIONS_H

/***********************************************/
/*     Cryptography Functions and Wrappers     */
/***********************************************/

void SHA256Hash(u8 *data, int dataLen, u8 *digest)
{
    SHA256((unsigned char *)data, dataLen, (unsigned char *)digest);
    return;
}

void SHA1HMAC(u8 *chal, int chalLen, u8 *key, int keyLen, u8 *resp)
{
    u32 respLen;
    HMAC(EVP_sha1(), (unsigned char *)key, keyLen, (unsigned char *)chal, chalLen, (unsigned char *)resp, &respLen);
    return;
}

void decryptAES128ECB(u8 *cryptText, int cryptLen, u8 *key, u8 *plainText)
{
    AES_KEY aeskey;
    AES_set_decrypt_key((unsigned char *)key, 128, &aeskey);
    AES_ecb_encrypt((unsigned char *)cryptText, (unsigned char *)plainText, &aeskey, AES_DECRYPT);
    return;
}

void decryptAES256CBC(u8 *cryptText, int cryptLen, u8 *key, u8 *IV, u8 *plainText)
{
    AES_KEY aeskey;
    u8 tmpKey[32], tmpIV[16];
    /* Copy key and IV to temp arrays to preserve original information */
    memcpy(tmpKey, key, 32);
    memcpy(tmpIV, IV, 16);
    AES_set_decrypt_key((unsigned char *)tmpKey, 256, &aeskey);
    AES_cbc_encrypt((unsigned char *)cryptText, (unsigned char *)plainText, cryptLen, &aeskey, (unsigned char *)tmpIV,
                    AES_DECRYPT);
    return;
}

u16 crc16(u8 *data, int dataLen)
{
    int i, j, k;
    u16 crc = 0xffff;
    for (i = 0; i < dataLen; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            k = crc & 1;
            crc >>= 1;
            if (k)
                crc ^= 0x8408;
        }
    }
    return crc;
}

#endif
