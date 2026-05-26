/**
 * Unit Tests: GRBL Status Parser
 *
 * Tests the parsing of GRBL state strings and status reports.
 */

#include <unity.h>
#include "testable_logic.h"

// ============================================================================
// Test: parseGrblState()
// ============================================================================

void test_parse_state_idle(void) {
    TEST_ASSERT_EQUAL(GrblState::Idle, parseGrblState("Idle"));
}

void test_parse_state_run(void) {
    TEST_ASSERT_EQUAL(GrblState::Run, parseGrblState("Run"));
}

void test_parse_state_hold(void) {
    TEST_ASSERT_EQUAL(GrblState::Hold, parseGrblState("Hold"));
}

void test_parse_state_hold_with_substate(void) {
    TEST_ASSERT_EQUAL(GrblState::Hold, parseGrblState("Hold:0"));
    TEST_ASSERT_EQUAL(GrblState::Hold, parseGrblState("Hold:1"));
}

void test_parse_state_jog(void) {
    TEST_ASSERT_EQUAL(GrblState::Jog, parseGrblState("Jog"));
}

void test_parse_state_alarm(void) {
    TEST_ASSERT_EQUAL(GrblState::Alarm, parseGrblState("Alarm"));
}

void test_parse_state_door(void) {
    TEST_ASSERT_EQUAL(GrblState::Door, parseGrblState("Door"));
    TEST_ASSERT_EQUAL(GrblState::Door, parseGrblState("Door:0"));
}

void test_parse_state_check(void) {
    TEST_ASSERT_EQUAL(GrblState::Check, parseGrblState("Check"));
}

void test_parse_state_home(void) {
    TEST_ASSERT_EQUAL(GrblState::Home, parseGrblState("Home"));
}

void test_parse_state_sleep(void) {
    TEST_ASSERT_EQUAL(GrblState::Sleep, parseGrblState("Sleep"));
}

void test_parse_state_unknown(void) {
    TEST_ASSERT_EQUAL(GrblState::Unknown, parseGrblState("FooBar"));
    TEST_ASSERT_EQUAL(GrblState::Unknown, parseGrblState(""));
    TEST_ASSERT_EQUAL(GrblState::Unknown, parseGrblState(nullptr));
}

// ============================================================================
// Test: parseGrblStatus()
// ============================================================================

void test_parse_status_basic_idle(void) {
    GrblStatus status;
    const char* line = "<Idle|WPos:0.000,0.000,0.000|Bf:15,128|FS:0,0>";

    TEST_ASSERT_TRUE(parseGrblStatus(line, status));
    TEST_ASSERT_EQUAL(GrblState::Idle, status.state);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, status.wposX);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, status.wposY);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, status.wposZ);
    TEST_ASSERT_EQUAL(15, status.bufferAvail);
    TEST_ASSERT_EQUAL(128, status.rxAvail);
    TEST_ASSERT_EQUAL(0, status.feedRate);
    TEST_ASSERT_EQUAL(0, status.spindleSpeed);
}

void test_parse_status_with_position(void) {
    GrblStatus status;
    const char* line = "<Run|WPos:10.500,25.250,-5.125|Bf:14,127|FS:500,1000>";

    TEST_ASSERT_TRUE(parseGrblStatus(line, status));
    TEST_ASSERT_EQUAL(GrblState::Run, status.state);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 10.5, status.wposX);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 25.25, status.wposY);
    TEST_ASSERT_FLOAT_WITHIN(0.001, -5.125, status.wposZ);
    TEST_ASSERT_EQUAL(500, status.feedRate);
    TEST_ASSERT_EQUAL(1000, status.spindleSpeed);
}

void test_parse_status_with_mpos(void) {
    // When MPos is provided instead of WPos, it should be copied to WPos
    GrblStatus status;
    const char* line = "<Jog|MPos:100.0,200.0,50.0|Bf:15,128>";

    TEST_ASSERT_TRUE(parseGrblStatus(line, status));
    TEST_ASSERT_EQUAL(GrblState::Jog, status.state);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 100.0, status.wposX);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 200.0, status.wposY);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 50.0, status.wposZ);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 100.0, status.mposX);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 200.0, status.mposY);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 50.0, status.mposZ);
}

void test_parse_status_with_overrides(void) {
    GrblStatus status;
    const char* line = "<Idle|WPos:0,0,0|Ov:80,90,100>";

    TEST_ASSERT_TRUE(parseGrblStatus(line, status));
    TEST_ASSERT_TRUE(status.overrides);
    TEST_ASSERT_EQUAL(80, status.feedOverride);
    TEST_ASSERT_EQUAL(90, status.rapidOverride);
    TEST_ASSERT_EQUAL(100, status.spindleOverride);
}

void test_parse_status_feed_only(void) {
    GrblStatus status;
    const char* line = "<Run|WPos:0,0,0|F:1500>";

    TEST_ASSERT_TRUE(parseGrblStatus(line, status));
    TEST_ASSERT_EQUAL(1500, status.feedRate);
}

void test_parse_status_invalid_empty(void) {
    GrblStatus status;
    TEST_ASSERT_FALSE(parseGrblStatus("", status));
    TEST_ASSERT_FALSE(parseGrblStatus(nullptr, status));
}

void test_parse_status_invalid_no_brackets(void) {
    GrblStatus status;
    TEST_ASSERT_FALSE(parseGrblStatus("Idle|WPos:0,0,0", status));
}

void test_parse_status_hold_substate(void) {
    GrblStatus status;
    const char* line = "<Hold:1|WPos:5,10,15|Bf:15,128>";

    TEST_ASSERT_TRUE(parseGrblStatus(line, status));
    TEST_ASSERT_EQUAL(GrblState::Hold, status.state);
}

void test_parse_status_negative_coordinates(void) {
    GrblStatus status;
    const char* line = "<Idle|WPos:-10.5,-20.25,-30.125|Bf:15,128|FS:100,0>";

    TEST_ASSERT_TRUE(parseGrblStatus(line, status));
    TEST_ASSERT_FLOAT_WITHIN(0.001, -10.5, status.wposX);
    TEST_ASSERT_FLOAT_WITHIN(0.001, -20.25, status.wposY);
    TEST_ASSERT_FLOAT_WITHIN(0.001, -30.125, status.wposZ);
}

// ============================================================================
// Test Runner
// ============================================================================

void setUp(void) {
    // Set up before each test (optional)
}

void tearDown(void) {
    // Clean up after each test (optional)
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // parseGrblState tests
    RUN_TEST(test_parse_state_idle);
    RUN_TEST(test_parse_state_run);
    RUN_TEST(test_parse_state_hold);
    RUN_TEST(test_parse_state_hold_with_substate);
    RUN_TEST(test_parse_state_jog);
    RUN_TEST(test_parse_state_alarm);
    RUN_TEST(test_parse_state_door);
    RUN_TEST(test_parse_state_check);
    RUN_TEST(test_parse_state_home);
    RUN_TEST(test_parse_state_sleep);
    RUN_TEST(test_parse_state_unknown);

    // parseGrblStatus tests
    RUN_TEST(test_parse_status_basic_idle);
    RUN_TEST(test_parse_status_with_position);
    RUN_TEST(test_parse_status_with_mpos);
    RUN_TEST(test_parse_status_with_overrides);
    RUN_TEST(test_parse_status_feed_only);
    RUN_TEST(test_parse_status_invalid_empty);
    RUN_TEST(test_parse_status_invalid_no_brackets);
    RUN_TEST(test_parse_status_hold_substate);
    RUN_TEST(test_parse_status_negative_coordinates);

    return UNITY_END();
}

