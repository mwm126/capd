/******************************************************/
/*    Core process that handles logging and crypto    */
/******************************************************/

void coreProcess(int coreToNetSocket, int coreToAuthSocket)
{
char netBuffer[BUFFER_SIZE];
FILE *lf,*cf,*pf;
int serial,destPort,timeout;
char username[32],destSystem[32];
u8 remoteAddr[addrSize], serverAddr[addrSize] = {0x00};
u8 AES128Key[16], AES256Key[32], HMACKey[20];
packet = (packet_t *)netBuffer;

lf = fopen(logFile,"a");
if (lf == NULL) fatal("SERVER HALT - Error opening log file.");
cf = fopen(counterFile,"r+");
if (cf == NULL) fatal("SERVER HALT - Error opening counter file.");
pf = fopen(passwdFile,"r");
if (pf == NULL) fatal("SERVER HALT - Error opening password file.");

/* Enter jail and drop privilege */
{
char processJailPath[MAX_PATH];
strcpy(processJailPath,jailPath);
strcat(processJailPath,"/core");
mkdir(processJailPath,rwX);
chown(processJailPath,0,0);
chmod(processJailPath,rwX);
chdir(processJailPath);
chroot(processJailPath);
setgid(gid);
setuid(uid);
if ((getgid() != gid) || (getuid() != uid))
	fatal("SERVER HALT - Privilege not dropped in Core process.");
}

/* Dump initialization info into log file */
logOutput(lf,0,"Starting CAPD");
logOutput(lf,0,"  Password File     : %s",passwdFile);
logOutput(lf,0,"  Counter File      : %s",counterFile);
logOutput(lf,0,"  Log File          : %s",logFile);
logOutput(lf,0,"  Jail Directory    : %s",jailPath);
logOutput(lf,0,"  Packet DeltaT     : %d",deltaT);
logOutput(lf,0,"  Login Timeout     : %d",initTimeout);
logOutput(lf,0,"  Spoof Timeout     : %d",spoofTimeout);
logOutput(lf,0,"  Firewall Script   : %s",openSSHPath);
logOutput(lf,0,"  Unprivileged User : %s",user);
logOutput(lf,0,"  UDP Port          : %d",port);
logOutput(lf,0,"  Interface Address : %s",serverAddress);
logOutput(lf,0,"  Verbosity Level   : %d",verbosity);
logOutput(lf,0,"  CAPD Drop Priv uid: %d",uid);
logOutput(lf,0,"  CAPD Drop Priv gid: %d",gid);

/* Parse server address */
{
struct sockaddr_in sa;
inet_pton(AF_INET,serverAddress,&(sa.sin_addr));
memcpy(serverAddr+12,&(sa.sin_addr),4);
}

while(1)
{
	/* Cryptographically parse and verify packet */
	if ((read(coreToNetSocket,remoteAddr,sizeof(remoteAddr)) != sizeof(remoteAddr)) ||
		(read(coreToNetSocket,netBuffer,BUFFER_SIZE) != sizeof(packet_t)))
		{
		logOutput(lf,0,"CRITICAL FAULT - Wrong-sized message received from Net Process!");
		continue;
		}
	serial = packet->serial;
	
	/* Look up user information */
	if (!(lookUpSerial(pf,serial,username,destSystem,&destPort,
			AES128Key,HMACKey)))
		{
		logOutput(lf,1,"Failed serial lookup: %d",serial);
		continue;
		}

	/* Construct SHA1-HMAC response and AES256 Key */
	{
	u8 response[20],tmp[100];
	SHA1HMAC(packet->challenge,32,HMACKey,20,response);
	memcpy(tmp,response,20);
	memcpy(tmp+20,packet->challenge,32);
	SHA256Hash(tmp,20+32,AES256Key);
	}
	
	/* Decrypt encBlock */
	decryptAES256CBC(packet->encBlock, sizeof(plain_t),
		AES256Key, packet->IV, plainBuffer);
	
	/* Verify plaintext chksum */
	{
	u8 digest[32];
	SHA256Hash(plainBuffer,chksumBlockSize,digest);
	if (memcmp((void*)digest,(void*)plain->chksum,chksumSize) != 0)
		{
		logOutput(lf,1,"Decrypted block failed chksum.  Serial: %d",serial);
		continue;
		}
	}
	
	/* Verify addresses */
	if (memcmp(plain->serverAddr,serverAddr,addrSize) != 0)
		{
		char str[33];
		logOutput(lf,1,
			"Bad server address in decrypted block.  Serial: %d",serial);
		arrayToHexString(serverAddr,str,16);
		logOutput(lf,2,"     Packet Value   : %s",str);
		arrayToHexString(plain->serverAddr,str,16);
		logOutput(lf,2,"     Decrypted Value: %s",str);
		continue;
		}
	if (memcmp(plain->authAddr,remoteAddr,addrSize) != 0)
		{
		char str[33];
		logOutput(lf,1,
			"Bad client address in decrypted block.  Serial: %d",serial);
		arrayToHexString(remoteAddr,str,16);
		logOutput(lf,2,"     Packet Value   : %s",str);
		arrayToHexString(plain->authAddr,str,16);
		logOutput(lf,2,"     Decrypted Value: %s",str);
		continue;
		}

	/* Verify username */
	if (strcmp(plain->username,username) != 0)
		{
		logOutput(lf,1,"Username verification failed: %s",plain->username);
		continue;
		}
	
	/* Verify challenge construction */
	{
	u8 tmp[100],digest[32];
	memcpy(tmp,"SHA1-HMACChallenge",18);
	memcpy(tmp+18,plain->OTP,16);
	memcpy(tmp+18+16,plain->entropy,32);
	SHA256(tmp,18+16+32,digest);	
	if (memcmp(packet->challenge,digest,SHA256_DIGEST_LENGTH) != 0)
		{
		logOutput(lf,1,
			"Challenge construction verification failed.  Serial: %d", serial);
		continue;
		}
	}

	/* Verify IV construction */	
	{
	u8 tmp[100],digest[32];
	memcpy(tmp,plain->entropy,32);
	memcpy(tmp+32,plain->OTP,16);
	SHA256Hash(tmp,32+16,digest);
	if (memcmp(packet->IV,digest,16) != 0)
		{
		logOutput(lf,1,
			"IV construction verification failed.  Serial: %d", serial);
		continue;
		}
	}
	
	/* Verify OTP */
	{			
	u8 OTPBuffer[sizeof(OTP_t)],tmp[100],digest[32];
	int counter;
	OTP_t *OTP = (OTP_t *)OTPBuffer;
	decryptAES128ECB(plain->OTP, sizeof(OTP_t), AES128Key, OTPBuffer);

	/* Verify decrypted CRC */
	if (0xf0b8 != crc16(OTPBuffer,16))
		{
		logOutput(lf,1,"OTP CRC verification failed.  Serial: %d",serial);
		continue;
		}

	/* Check OTP Challenge */
	memcpy(tmp,"yubicoChal",10);
	memcpy(tmp+10,plain->entropy,32);
	SHA256(tmp,10+32,digest);
	if ((memcmp(digest,OTP->OTPChallenge,6) != 0))
		{
		logOutput(lf,1,"OTP Challenge verification failed.  Serial: %d",serial);
		continue;
		}

	counter = OTP->insertCounter*256 + OTP->sessionCounter;
	/* Check counter */
	if (counter <= searchCounterFile(cf,serial))
		{
		logOutput(lf,1,"OTP Counter verification failed.  Serial: %d",serial);
		continue;
		}
	updateCounterFileEntry(cf,serial,counter);
	logOutput(lf,1,"Accepted connection: %s, %d, %d",username,serial,counter);
	}
	
	/* Setup timeout */
	if (memcmp(plain->connAddr,plain->authAddr,addrSize) == 0)
		timeout = initTimeout;
	else
		timeout = spoofTimeout;
	
	{
	/* Translate connIPbinary to IP as text */
	char connAddrTxt[addrTxtSize]={0x00};
	u8 allZeros[12]={0x00};
	if (memcmp(plain->connAddr,allZeros,12) == 0)	/* IPV4 */
		{
		u8 ipv4[4];
		memcpy(ipv4,plain->connAddr+12,4);		
		inet_ntop(AF_INET,ipv4,connAddrTxt,INET_ADDRSTRLEN);
		}
	else	/* IPV6 */
		inet_ntop(AF_INET6,plain->connAddr,connAddrTxt,INET6_ADDRSTRLEN);

	/* Authorize connection */
	write(coreToAuthSocket,connAddrTxt,addrTxtSize);
	write(coreToAuthSocket,destSystem,sizeof(destSystem));
	write(coreToAuthSocket,&destPort,sizeof(destPort));
	write(coreToAuthSocket,&timeout,sizeof(timeout));		
	}
}
return;
}