/**
 * Unit Tests: Continuous Jog Segments
 *
 * Tests for calcJogSegment() — the per-segment move streamed while a jog
 * button is held.
 */

#include <unity.h>
#include <cmath>
#include "testable_logic.h"

static float dx, dy, dz;

// ============================================================================
// Test: straight moves
// ============================================================================

void test_jog_segment_x_plus(void) {
    // 1000 mm/min for 100 ms = 1.6667 mm
    TEST_ASSERT_TRUE(calcJogSegment(1, 0, 0, 1000.0f, 100, dx, dy, dz));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.6667f, dx);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dy);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dz);
}

void test_jog_segment_y_minus(void) {
    TEST_ASSERT_TRUE(calcJogSegment(0, -1, 0, 600.0f, 100, dx, dy, dz));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dx);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, dy);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dz);
}

void test_jog_segment_z_plus(void) {
    TEST_ASSERT_TRUE(calcJogSegment(0, 0, 1, 500.0f, 120, dx, dy, dz));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, dz);
}

void test_jog_segment_scales_with_duration(void) {
    TEST_ASSERT_TRUE(calcJogSegment(1, 0, 0, 3000.0f, 200, dx, dy, dz));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 10.0f, dx);
}

// ============================================================================
// Test: diagonal moves keep path speed equal to the feed
// ============================================================================

void test_jog_segment_diagonal_path_length(void) {
    TEST_ASSERT_TRUE(calcJogSegment(1, -1, 0, 1000.0f, 100, dx, dy, dz));
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.6667f, len);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, dx, -dy);
    TEST_ASSERT_TRUE(dx > 0.0f);
}

void test_jog_segment_three_axis_path_length(void) {
    TEST_ASSERT_TRUE(calcJogSegment(-1, 1, 1, 1200.0f, 100, dx, dy, dz));
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, len);
}

void test_jog_segment_clamps_direction(void) {
    // Direction magnitude is ignored — only the sign counts
    TEST_ASSERT_TRUE(calcJogSegment(5, 0, 0, 1000.0f, 100, dx, dy, dz));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.6667f, dx);
}

// ============================================================================
// Test: invalid input
// ============================================================================

void test_jog_segment_zero_direction(void) {
    dx = dy = dz = 99.0f;
    TEST_ASSERT_FALSE(calcJogSegment(0, 0, 0, 1000.0f, 100, dx, dy, dz));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dx);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dy);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dz);
}

void test_jog_segment_zero_feed(void) {
    TEST_ASSERT_FALSE(calcJogSegment(1, 0, 0, 0.0f, 100, dx, dy, dz));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dx);
}

void test_jog_segment_negative_feed(void) {
    TEST_ASSERT_FALSE(calcJogSegment(1, 0, 0, -500.0f, 100, dx, dy, dz));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dx);
}

void test_jog_segment_zero_duration(void) {
    TEST_ASSERT_FALSE(calcJogSegment(1, 0, 0, 1000.0f, 0, dx, dy, dz));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, dx);
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_jog_segment_x_plus);
    RUN_TEST(test_jog_segment_y_minus);
    RUN_TEST(test_jog_segment_z_plus);
    RUN_TEST(test_jog_segment_scales_with_duration);

    RUN_TEST(test_jog_segment_diagonal_path_length);
    RUN_TEST(test_jog_segment_three_axis_path_length);
    RUN_TEST(test_jog_segment_clamps_direction);

    RUN_TEST(test_jog_segment_zero_direction);
    RUN_TEST(test_jog_segment_zero_feed);
    RUN_TEST(test_jog_segment_negative_feed);
    RUN_TEST(test_jog_segment_zero_duration);

    return UNITY_END();
}
