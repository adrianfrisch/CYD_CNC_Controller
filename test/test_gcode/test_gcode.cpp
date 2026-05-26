/**
 * Unit Tests: GCode Processing
 *
 * Tests the GCode comment stripping and line processing functions.
 */

#include <unity.h>
#include "testable_logic.h"
#include <cstring>

// ============================================================================
// Test: stripGCodeComments()
// ============================================================================

void test_strip_parenthetical_comment(void) {
    char line[] = "G1 X10 (move to position)";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G1 X10", line);
}

void test_strip_semicolon_comment(void) {
    char line[] = "G1 Y5 ; this is a comment";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G1 Y5", line);
}

void test_strip_leading_whitespace(void) {
    char line[] = "  G0 Z0";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G0 Z0", line);
}

void test_strip_trailing_whitespace(void) {
    char line[] = "M3 S1000   ";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("M3 S1000", line);
}

void test_strip_full_comment_line_semicolon(void) {
    char line[] = "; This is a comment";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("", line);
}

void test_strip_full_comment_line_parenthesis(void) {
    char line[] = "(comment only)";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("", line);
}

void test_strip_mixed_whitespace_and_comments(void) {
    char line[] = "  G1 X5 (move) ; end comment  ";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G1 X5", line);
}

void test_strip_no_modification_needed(void) {
    char line[] = "G1X10Y20Z30";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G1X10Y20Z30", line);
}

void test_strip_tabs(void) {
    char line[] = "\t\tG90\t";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G90", line);
}

void test_strip_only_whitespace(void) {
    char line[] = "   \t   ";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("", line);
}

void test_strip_empty_line(void) {
    char line[] = "";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("", line);
}

void test_strip_null_safe(void) {
    // Should not crash on nullptr
    stripGCodeComments(nullptr);
    TEST_PASS();
}

void test_strip_parenthesis_at_start(void) {
    char line[] = "(comment)G1 X10";
    stripGCodeComments(line);
    // Parenthesis at start removes everything after
    TEST_ASSERT_EQUAL_STRING("", line);
}

void test_strip_two_comments(void) {
    // First comment type should stop processing, so only first applies
    char line[] = "G1 (a) X10 ; b";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G1", line);
}

void test_strip_complex_gcode(void) {
    char line[] = "G1 X-10.5 Y20.25 Z-5.125 F1000 (rapid move)";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("G1 X-10.5 Y20.25 Z-5.125 F1000", line);
}

void test_strip_spindle_command(void) {
    char line[] = "M3 S12000 ; start spindle at 12000 RPM";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("M3 S12000", line);
}

void test_strip_tool_change(void) {
    char line[] = "M6 T1 (tool change to tool #1)";
    stripGCodeComments(line);
    TEST_ASSERT_EQUAL_STRING("M6 T1", line);
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

    RUN_TEST(test_strip_parenthetical_comment);
    RUN_TEST(test_strip_semicolon_comment);
    RUN_TEST(test_strip_leading_whitespace);
    RUN_TEST(test_strip_trailing_whitespace);
    RUN_TEST(test_strip_full_comment_line_semicolon);
    RUN_TEST(test_strip_full_comment_line_parenthesis);
    RUN_TEST(test_strip_mixed_whitespace_and_comments);
    RUN_TEST(test_strip_no_modification_needed);
    RUN_TEST(test_strip_tabs);
    RUN_TEST(test_strip_only_whitespace);
    RUN_TEST(test_strip_empty_line);
    RUN_TEST(test_strip_null_safe);
    RUN_TEST(test_strip_parenthesis_at_start);
    RUN_TEST(test_strip_two_comments);
    RUN_TEST(test_strip_complex_gcode);
    RUN_TEST(test_strip_spindle_command);
    RUN_TEST(test_strip_tool_change);

    return UNITY_END();
}

