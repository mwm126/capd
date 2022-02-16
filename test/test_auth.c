#include "unity.h"

#include "authProcess.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_build_command_line(void)
{
    char commandLine[MAX_PATH + 150];
    char allowedAddr[addrSize] = "192.168.10.200";
    char hostAddr[32] = "99.8.77.66";
    char destAddr[32] = "44.33.22.11";
    int destPort = 54321;
    int timeLimit = 12345;

    build_command_line(commandLine, allowedAddr, hostAddr, timeLimit, destAddr, destPort);

    TEST_ASSERT_EQUAL_STRING("/usr/sbin/openClose.sh 192.168.10.200 99.8.77.66 54321 12345 44.33.22.11 &", commandLine);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_build_command_line);
    return UNITY_END();
}
