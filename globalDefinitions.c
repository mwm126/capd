#include "globalDefinitions.h"
#include "utilityFunctions.h"

/* Global Control Variables */
char _capdVersion[40] = "1.03-dev";
char _passwdFile[MAX_PATH] = "/etc/capd/capd.passwd";
char _counterFile[MAX_PATH] = "/etc/capd/capd.counter";
char _logFile[MAX_PATH] = "/var/log/capd.log";
char _jailPath[MAX_PATH] = "/tmp/capd/";
char _openSSHPath[MAX_PATH] = "/usr/sbin/openClose.sh";
char _serverAddress[MAX_NO_OF_SERVER_ADDR][32] = {{0x00}};
int _noOfServerAddresses = 0;
u32 _deltaT = 30;       /* Maximum allowed clock skew from client to server */
u32 _initTimeout = 5;   /* Maximum time allowed to open local ssh connection */
u32 _spoofTimeout = 30; /* Maximum time allowed to open spoofed ssh connection */
char _user[32] = "capd";
uid_t _uid, _gid;
int _port = 62201;
int _verbosity = 1;

char *capdVersion()
{
    return _capdVersion;
}

char *passwdFile()
{
    return _passwdFile;
}

char *counterFile()
{
    return _counterFile;
}

char *logFile()
{
    return _logFile;
}
char *jailPath()
{
    return _jailPath;
}
char *openSSHPath()
{
    return _openSSHPath;
}

char *serverAddress(int n)
{
    return _serverAddress[n];
}

int noOfServerAddresses()
{
    return _noOfServerAddresses;
}

u32 deltaT()
{
    return _deltaT;
}

u32 initTimeout()
{
    return _initTimeout;
}

u32 spoofTimeout()
{
    return _spoofTimeout;
}

char *user()
{
    return _user;
}

uid_t uid()
{
    return _uid;
}

uid_t gid()
{
    return _gid;
}

int capPort()
{
    return _port;
}

int verbosity()
{
    return _verbosity;
}

void init_usage(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            printf("capd - Cloaked Access Protocol Daemon - Version %s\n\n", _capdVersion);
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
            exit(0);
        }
        if ((strcmp(argv[i], "-pw") == 0) || (strcmp(argv[i], "--password-file") == 0))
            strcpy(_passwdFile, argv[++i]);
        if ((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--counter-file") == 0))
            strcpy(_counterFile, argv[++i]);
        if ((strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "--log-file") == 0))
            strcpy(_logFile, argv[++i]);
        if ((strcmp(argv[i], "-j") == 0) || (strcmp(argv[i], "--jail-dir") == 0))
            strcpy(_jailPath, argv[++i]);
        if ((strcmp(argv[i], "-dt") == 0) || (strcmp(argv[i], "--deltat") == 0))
            _deltaT = atoi(argv[++i]);
        if ((strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--timeout") == 0))
            _initTimeout = atoi(argv[++i]);
        if ((strcmp(argv[i], "-q") == 0) || (strcmp(argv[i], "--spoof-timeout") == 0))
            _spoofTimeout = atoi(argv[++i]);
        if ((strcmp(argv[i], "-u") == 0) || (strcmp(argv[i], "--user") == 0))
            strcpy(_user, argv[++i]);
        if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--script") == 0))
            strcpy(_openSSHPath, argv[++i]);
        if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--port") == 0))
            _port = atoi(argv[++i]);
        if (((strcmp(argv[i], "-a") == 0) || (strcmp(argv[i], "--address") == 0)) &&
            (noOfServerAddresses() < MAX_NO_OF_SERVER_ADDR - 1))
            strcpy(_serverAddress[_noOfServerAddresses++], argv[++i]);
        if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--verbosity") == 0))
            _verbosity = atoi(argv[++i]);
        _verbosity = BOUND(_verbosity, 0, 2);
    }

    /* Security Checks and File/Directory Setup */
    {
        struct passwd *pw;
        pw = getpwnam(_user);
        if (pw == NULL)
        {
            fatal("SERVER HALT - User not found");
            return;
        }
        _uid = pw->pw_uid;
        _gid = pw->pw_gid;
        mkdir(_jailPath, rwX);
        chown(_jailPath, 0, 0);
        chmod(_jailPath, rwX);
        chown(_passwdFile, 0, 0);
        chmod(_passwdFile, RWx);
        chown(_counterFile, 0, 0);
        chmod(_counterFile, RWx);
    }
}
