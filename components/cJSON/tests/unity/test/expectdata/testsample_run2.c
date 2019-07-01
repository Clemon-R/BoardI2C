/* AUTOGENERATED FILE. DO NOT EDIT. */

/*=======Test Runner Used To Run Each Test Below=====*/
#define RUN_TEST(TestFunc, TestLineNum) \
{ \
  Unity.CurrentTestName = #TestFunc; \
  Unity.CurrentTestLineNumber = TestLineNum; \
  Unity.NumberOfTests++; \
  if (TEST_PROTECT()) \
  { \
      setUp(); \
      TestFunc(); \
  } \
  if (TEST_PROTECT() && !TEST_IS_IGNORED) \
  { \
    tearDown(); \
  } \
  UnityConcludeTest(); \
}

/*=======Automagically Detected Files To Include=====*/
#include "unity.h"
#include <setjmp.h>
#include <stdio.h>
#include "funky.h"
#include "stanky.h"
#include <setjmp.h>

/*=======External Functions This Runner Calls=====*/
extern void setUp(void);
extern void tearDown(void);
extern void test_TheFirstThingToTest(void);
extern void test_TheSecondThingToTest(void);
extern void test_TheThirdThingToTest(void);
extern void test_TheFourthThingToTest(void);


/*=======Suite Setup=====*/
static int suite_setup(void)
{
    a_custom_setup();
}

/*=======Suite Teardown=====*/
static int suite_teardown(int num_failures)
{
    a_custom_teardown();
}

/*=======Test Reset Option=====*/
void resetTest(void);
void resetTest(void)
{
    tearDown();
    setUp();
}


/*=======MAIN=====*/
int main(void)
{
    suite_setup();
    UnityBegin("testdata/testsample.c");
    RUN_TEST(test_TheFirstThingToTest, 21);
    RUN_TEST(test_TheSecondThingToTest, 43);
    RUN_TEST(test_TheThirdThingToTest, 53);
    RUN_TEST(test_TheFourthThingToTest, 58);

    return suite_teardown(UnityEnd());
}
