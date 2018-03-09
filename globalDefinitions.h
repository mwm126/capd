#ifndef GLOBALDEFINITIONS_H
#define GLOBALDEFINITIONS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_PATH 4096
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define BOUND(X, L, H) MAX(L, MIN(H, X))

enum { FALSE, TRUE };
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Static sizes */
#define BUFFER_SIZE 1024
#define MAX_NO_OF_CONNECTIONS 2000
#define addrSize 16
#define OTPSize 16
#define chksumSize SHA256_DIGEST_LENGTH
#define MACSize SHA256_DIGEST_LENGTH
#define addrTxtSize INET6_ADDRSTRLEN

/* File permission macros */
#define RWx 384
#define rwX 64

/* Global Control Variables */
char passwdFile[MAX_PATH] = "/etc/capd/capd.passwd";
char counterFile[MAX_PATH] = "/etc/capd/capd.counter";
char logFile[MAX_PATH] = "/var/log/capd.log";
char jailPath[MAX_PATH] = "/tmp/capd/";
char openSSHPath[MAX_PATH] = "/usr/sbin/openClose.sh";
char serverAddress[32];
u32 deltaT = 30;       /* Maximum allowed clock skew from client to server */
u32 initTimeout = 5;   /* Maximum time allowed to open local ssh connection */
u32 spoofTimeout = 30; /* Maximum time allowed to open spoofed ssh connection */
char user[32] = "capd";
unsigned int uid, gid;
int port = 62201;
int verbosity = 1;

/* Packet, Payload, and OTP Structures and Buffers */
typedef struct {
  u8 authAddr[addrSize];
  u8 connAddr[addrSize];
  u8 serverAddr[addrSize];
  u8 OTP[OTPSize];
  char username[32];
  u8 entropy[32];
  u8 chksum[chksumSize];
} plain_t;
u8 plainBuffer[sizeof(plain_t)];
plain_t *plain = (plain_t *)plainBuffer;
#define chksumBlockSize (sizeof(plain_t) - chksumSize)

typedef struct {
  int timestamp;
  int serial;
  u8 IV[16];
  u8 challenge[32];
  u8 encBlock[sizeof(plain_t)];
  u8 MAC[32];
} packet_t;

packet_t *packet = NULL;

#define MACBlockSize (sizeof(packet_t) - MACSize)

typedef struct {
  u8 OTPChallenge[6];
  u16 insertCounter;
  u8 timestamp[3];
  u8 sessionCounter;
  u16 random;
  u16 crc;
} OTP_t;

#endif
