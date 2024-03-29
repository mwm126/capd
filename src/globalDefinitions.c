#include "globalDefinitions.h"
#include "parse_config.h"
#include "utilityFunctions.h"

#define STRINGIZE(x) #x
#define QUOTE(x) STRINGIZE(x)

/* Global Control Variables */
char _capdVersion[40];
char _passwdFile[MAX_PATH];
char _counterFile[MAX_PATH];
char _logFile[MAX_PATH];
char _jailPath[MAX_PATH];
char _openSSHPath[MAX_PATH];
char _serverAddress[MAX_NO_OF_SERVER_ADDR][32];
int _noOfServerAddresses;
u32 _deltaT;       /* Maximum allowed clock skew from client to server */
u32 _initTimeout;  /* Maximum time allowed to open local ssh connection */
u32 _spoofTimeout; /* Maximum time allowed to open spoofed ssh connection */
char _user[32];
uid_t _uid, _gid;
int _port;
bool _ignore_pkt_ip;
int _verbosity;

void init_globals()
{
    strcpy(_capdVersion, QUOTE(CAPD_VERSION));
    strcpy(_passwdFile, "/etc/capd/capd.passwd");
    strcpy(_counterFile, "/etc/capd/capd.counter");
    strcpy(_logFile, "/var/log/capd.log");
    strcpy(_jailPath, "/var/capd/");
    strcpy(_openSSHPath, "/usr/sbin/openClose.sh");
    _noOfServerAddresses = 0;
    _deltaT = 30;       /* Maximum allowed clock skew from client to server */
    _initTimeout = 5;   /* Maximum time allowed to open local ssh connection */
    _spoofTimeout = 30; /* Maximum time allowed to open spoofed ssh connection */
    strcpy(_user, "capd");
    _port = 62201;
    _ignore_pkt_ip = false;
    _verbosity = 1;
}

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

bool ignore_pkt_ip()
{
    return _ignore_pkt_ip;
}

int verbosity()
{
    return _verbosity;
}

void init_usage(int argc, char *argv[])
{
    init_globals();
    char cfg_fname[1000];
    strcpy(cfg_fname, "/etc/capd/capd.toml");

    int i;
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            printf("capd - Cloaked Access Protocol Daemon - %s\n\n", _capdVersion);
            printf("  Usage:\n");
            printf("  -g,  --config-file [capd config file; default /etc/capd/capd.toml]\n");
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
            printf("  -i,  --ignore-pkt-ip [Ignore CAP packet IP address mismatch]\n");
            printf("  -v,  --verbosity [verbosity level]\n\n");
            exit(0);
        }
        if ((strcmp(argv[i], "-g") == 0) || (strcmp(argv[i], "--config-file") == 0))
            strcpy(cfg_fname, argv[++i]);
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

        if (((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--ignore-pkt-in") == 0)))
            _ignore_pkt_ip = true;
        if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--verbosity") == 0))
            _verbosity = atoi(argv[++i]);
        _verbosity = BOUND(_verbosity, 0, 2);
    }

    config_t cfg = init_config(cfg_fname);
    if (strcmp(cfg.address, ""))
    {
        strcpy(_serverAddress[_noOfServerAddresses++], cfg.address);
    }
}

void setup_security(uid_t uid, gid_t gid)
/* Security Checks and File/Directory Setup */
{
    struct passwd *pw = getpwnam(_user);
    if (pw == NULL)
    {
        char msg[1000];
        sprintf(msg, "SERVER HALT - User not found: %s", _user);
        fatal(msg);
        return;
    }
    _uid = pw->pw_uid;
    _gid = pw->pw_gid;
    ABORT_IF_ERR(chown(_jailPath, uid, gid), "Could not chown jail path");
    ABORT_IF_ERR(chmod(_jailPath, rwX), "Could not chmod jail path");
    ABORT_IF_ERR(chown(_passwdFile, uid, gid), "Could not chown passwd");
    ABORT_IF_ERR(chmod(_passwdFile, RWx), "Could not chmod passwd");
    ABORT_IF_ERR(chown(_counterFile, uid, gid), "Could not chown counter file");
    ABORT_IF_ERR(chmod(_counterFile, RWx), "Could not chmod counter file");
}
