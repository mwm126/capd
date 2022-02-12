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
    char allowedAddr[addrTxtSize] = "192.168.10.200";
    char hostAddr[32] = "99.8.77.66";
    int timeLimit = 12345;
    build_command_line(commandLine, allowedAddr, hostAddr, timeLimit);
    TEST_ASSERT_EQUAL_STRING("/usr/sbin/openClose.sh 192.168.10.200 99.8.77.66 62201 12345 &", commandLine);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_build_command_line);
    return UNITY_END();
}
