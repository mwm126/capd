/***************************************************************/
/*  Cloaked Access Protocol Daemon                             */
/*  (C) 2012-2013 Aeolus Technologies, Inc.                    */
/*  All Rights Reserved.                                       */
/*                                                             */
/*  Build command: gcc -lrt -lcrypto -O3 -Wall -o capd capd.c  */
/***************************************************************/

#include "globalDefinitions.h"
#include "utilityFunctions.h"
#include "counterFunctions.h"
#include "cryptoFunctions.h"
#include "netProcess.h"
#include "coreProcess.h"
#include "authProcess.h"

int main (int argc, char *argv[])
{
/* Test endianness */
identifyEndianess();

/* Parse commandline inputs */
{
int i;
for (i=1;i<argc;i++)
	{
	if (strcmp(argv[i],"-h")==0)
		{
		printf("capd - Cloaked Access Protocol Daemon - Version %s\n\n",capdVersion);
		printf("  Usage:\n");
		printf("  -pw, --password-file [capd password file]\n");
		printf("  -c,  --counter-file [capd counter file]\n"); 
		printf("  -l,  --log-file [capd log file]\n");
		printf("  -j,  --jail-dir [capd jail directory]\n");
		printf("  -dt, --deltat [maximum packet timestamp variation in seconds]\n");
		printf("  -t,  --timeout [time limit for connection init in seconds]\n");
		printf("  -q,  --spoof-timeout [time limit for spoofed connection init]\n");
		printf("  -u,  --user [user for drop privilege]\n");
		printf("  -s,  --script [fully resolved path to firewall script]\n");
		printf("  -p,  --port [UDP port]\n");
		printf("  -a,  --address [Server interface IP address]\n");
		printf("  -v,  --verbosity [verbosity level]\n\n");
		return 0;
		}
	if ((strcmp(argv[i],"-pw")==0) || (strcmp(argv[i],"--password-file")==0))
		strcpy(passwdFile,argv[++i]);	
	if ((strcmp(argv[i],"-c")==0)  || (strcmp(argv[i],"--counter-file")==0))
		strcpy(counterFile,argv[++i]);
	if ((strcmp(argv[i],"-l")==0)  || (strcmp(argv[i],"--log-file")==0))
		strcpy(logFile,argv[++i]);
	if ((strcmp(argv[i],"-j")==0)  || (strcmp(argv[i],"--jail-dir")==0))
		strcpy(jailPath,argv[++i]);
	if ((strcmp(argv[i],"-dt")==0) || (strcmp(argv[i],"--deltat")==0))
		deltaT=atoi(argv[++i]);
	if ((strcmp(argv[i],"-t")==0)  || (strcmp(argv[i],"--timeout")==0))
		initTimeout=atoi(argv[++i]);
	if ((strcmp(argv[i],"-q")==0)  || (strcmp(argv[i],"--spoof-timeout")==0))
		spoofTimeout=atoi(argv[++i]);
	if ((strcmp(argv[i],"-u")==0)  || (strcmp(argv[i],"--user")==0))
		strcpy(user,argv[++i]);
	if ((strcmp(argv[i],"-s")==0)  || (strcmp(argv[i],"--script")==0))
		strcpy(openSSHPath,argv[++i]);
	if ((strcmp(argv[i],"-p")==0)  || (strcmp(argv[i],"--port")==0))
		port=atoi(argv[++i]);
	if (    ((strcmp(argv[i],"-a")==0)  || (strcmp(argv[i],"--address")==0)) &&
            (noOfServerAddresses < MAX_NO_OF_SERVER_ADDR-1) )
		strcpy(serverAddress[noOfServerAddresses++],argv[++i]);
	if ((strcmp(argv[i],"-v")==0)  || (strcmp(argv[i],"--verbosity")==0))
		verbosity=atoi(argv[++i]);	
	verbosity = BOUND(verbosity,0,2);
	}
}

/* Security Checks and File/Directory Setup */
{
struct passwd *pw;
pw = getpwnam(user);
if (pw==NULL) fatal ("SERVER HALT - User not found");
uid=pw->pw_uid;
gid=pw->pw_gid;
mkdir(jailPath,rwX);
chown(jailPath,0,0);
chmod(jailPath,rwX);
chown(passwdFile,0,0);
chmod(passwdFile,RWx);
chown(counterFile,0,0);
chmod(counterFile,RWx);
}

/* Fork into processes and establish sockets connections */
{
int sockets[2];
int netPID, corePID;
int netToCoreSocket=0,coreToNetSocket=0;
int coreToAuthSocket=0,authToCoreSocket=0;

socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
netPID=fork();
if(!netPID)
	{
	netToCoreSocket=sockets[0];
	close(sockets[1]);
	netProcess(netToCoreSocket);
	return 0;
	}
coreToNetSocket=sockets[1];
close(sockets[0]);

socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
corePID=fork();
if(!corePID)
	{
	coreToAuthSocket=sockets[0];
	close(sockets[1]);
	coreProcess(coreToNetSocket,coreToAuthSocket);
	return 0;
	}
close(coreToNetSocket);
coreToNetSocket=0;
authToCoreSocket=sockets[1];
close(sockets[0]);

authProcess(authToCoreSocket,netPID,corePID);
}
return 0;
}
