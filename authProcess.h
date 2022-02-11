#ifndef AUTHPROCESS_H
#define AUTHPROCESS_H

/************************************/
/*   Authorization of connections   */
/************************************/

void authProcess(int authToCoreSocket, int netPID, int corePID)
{
char commandLine[MAX_PATH + 150];
char tmp[15];
char hostAddr[32];
char allowedAddr[addrTxtSize];
int i,port,timeLimit;

/* This process retains privilege and is unjailed. */
while(1)
{
	/* Wait for authorization information */
	if	(
		(read(authToCoreSocket,allowedAddr,sizeof(allowedAddr)) != sizeof(allowedAddr)) ||
		(read(authToCoreSocket,hostAddr,sizeof(hostAddr)) != sizeof(hostAddr)) ||
		(read(authToCoreSocket,&port,sizeof(port)) != sizeof(port)) ||
		(read(authToCoreSocket,&timeLimit,sizeof(timeLimit)) != sizeof(timeLimit))
		)
		/* Kill all capd processes if wrong-sized message arrives. */
		{
		kill(netPID,SIGKILL);
		kill(corePID,SIGKILL);
		fatal("CRITICAL FAULT - Wrong-sized message received from Auth Process!");
		}
		
	/* Sanitize allowedAddr and hostAddr to prevent script hijacking */
	for (i=0;i<sizeof(allowedAddr);i++)
		if	(!
				(	(allowedAddr[i] == 0)  ||	/* Null OK */
					(allowedAddr[i] == 46) ||	/* Period OK */
					(allowedAddr[i] == 58) ||	/* Colon OK (IPV6) */
					((allowedAddr[i] >= 48) && (allowedAddr[i] <= 57)) ||	/* 0-9 OK */
					((allowedAddr[i] >= 97) && (allowedAddr[i] <= 102)) ||	/* a-f OK (IPV6) */
					((allowedAddr[i] >= 65) && (allowedAddr[i] <= 70))   	/* A-F OK (IPV6) */
				)
			) allowedAddr[i] = 46;	/* Replace bad character with period */
	for (i=0;i<sizeof(hostAddr);i++)
		if	(!
				(	(hostAddr[i] == 0)  ||	/* Null OK */
					(hostAddr[i] == 46) ||	/* Period OK */
					(hostAddr[i] == 58) ||	/* Colon OK (IPV6) */
					(hostAddr[i] == 45) || (hostAddr[i] == 95)  ||		/* hyphen and underscore */
					((hostAddr[i] >= 48) && (hostAddr[i] <= 57)) || 	/* 0-9 OK */
					((hostAddr[i] >= 97) && (hostAddr[i] <= 122)) ||	/* a-z OK */
					((hostAddr[i] >= 65) && (hostAddr[i] <= 90))		/* A-Z OK */
				)
			) hostAddr[i] = 46;	/* Replace bad character with period */

	/* Place hard limits on int values */
	timeLimit = BOUND(timeLimit,0,300);
	port = BOUND(port,0,65535);

	/* Build command line for openSSH.sh call */
	memset(commandLine,0,sizeof(commandLine));
	strncpy(commandLine,openSSHPath,sizeof(openSSHPath));
	strncat(commandLine," ",1);
	strncat(commandLine,allowedAddr,sizeof(allowedAddr));
	strncat(commandLine," ",1);
	strncat(commandLine,hostAddr,sizeof(hostAddr));
	strncat(commandLine," ",1);	
	sprintf(tmp,"%d",port);
	strncat(commandLine,tmp,sizeof(tmp));
	strncat(commandLine," ",1);
	sprintf(tmp,"%d",timeLimit);
	strncat(commandLine,tmp,sizeof(tmp));
	strncat(commandLine," &",2);

	/* Execute openSSH.sh command */
	system(commandLine);
//	printf("%s\n",commandLine);
}
return;
}


#endif
