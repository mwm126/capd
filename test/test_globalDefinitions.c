#include "unity.h"

#include "globalDefinitions.h"

char *CAPD_TEST_USER;

void setUp(void)
{
    CAPD_TEST_USER = getenv("USER");
}
void tearDown(void)
{
}

void test_init_usage_defaults(void)
{
    const int argc = 3;
    char *argv[] = {"/usr/local/bin/capd", "-u", CAPD_TEST_USER};

    init_usage(argc, argv);

    TEST_ASSERT_EQUAL_STRING("/etc/capd/capd.passwd", passwdFile());
    TEST_ASSERT_EQUAL_STRING("/etc/capd/capd.counter", counterFile());
    TEST_ASSERT_EQUAL_STRING("/var/log/capd.log", logFile());
    TEST_ASSERT_EQUAL_STRING("/tmp/capd/", jailPath());
    TEST_ASSERT_EQUAL_INT(30, deltaT());
    TEST_ASSERT_EQUAL_INT(5, initTimeout());
    TEST_ASSERT_EQUAL_INT(30, spoofTimeout());
    TEST_ASSERT_EQUAL_STRING("/usr/sbin/openClose.sh", openSSHPath());
    TEST_ASSERT_EQUAL_STRING(CAPD_TEST_USER, user());
    TEST_ASSERT_EQUAL_INT(62201, capPort());
}

void test_init_usage_cmdline(void)
{
    const int argc = 21;
    char *argv[] = {
        "/usr/local/bin/capd", "-pw", "capd.pwd", "-c", "capd.counter", "-l", "capd.log", "-v",  "2",   "-u",
        CAPD_TEST_USER,        "-q",  "777",      "-p", "999",          "-t", "456",      "-dt", "123", "-s",
        "/open/close.sh"};

    init_usage(argc, argv);

    TEST_ASSERT_EQUAL_STRING("capd.pwd", passwdFile());
    TEST_ASSERT_EQUAL_STRING("capd.counter", counterFile());
    TEST_ASSERT_EQUAL_STRING("capd.log", logFile());
    TEST_ASSERT_EQUAL_STRING("capd.pwd", passwdFile());
    TEST_ASSERT_EQUAL_STRING("/tmp/capd/", jailPath());
    TEST_ASSERT_EQUAL_INT(123, deltaT());
    TEST_ASSERT_EQUAL_INT(456, initTimeout());
    TEST_ASSERT_EQUAL_INT(777, spoofTimeout());
    TEST_ASSERT_EQUAL_STRING("/open/close.sh", openSSHPath());
    TEST_ASSERT_EQUAL_STRING(CAPD_TEST_USER, user());
    TEST_ASSERT_EQUAL_INT(999, capPort());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_usage_defaults);
    RUN_TEST(test_init_usage_cmdline);
    return UNITY_END();
}
