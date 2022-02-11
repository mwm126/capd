#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

void test_hi(void)
{
    TEST_IGNORE_MESSAGE("Hello world!");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hi);
    return UNITY_END();
}
