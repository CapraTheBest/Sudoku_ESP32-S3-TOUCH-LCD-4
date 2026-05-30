#include <unity.h>
#include "sudoku_types.h"

void setUp(void) {}
void tearDown(void) {}

void test_scaffolding_builds(void) {
    TEST_ASSERT_EQUAL_INT(81, sudoku::CELLS);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_scaffolding_builds);
    return UNITY_END();
}
