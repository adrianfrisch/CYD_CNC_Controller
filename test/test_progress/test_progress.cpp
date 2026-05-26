/**
 * Unit Tests: Progress Calculations
 *
 * Tests for job progress percentage and ETA calculations.
 */

#include <unity.h>
#include "testable_logic.h"

// ============================================================================
// Test: calculateProgress()
// ============================================================================

void test_progress_zero_percent(void) {
    TEST_ASSERT_EQUAL(0, calculateProgress(0, 1000));
}

void test_progress_fifty_percent(void) {
    TEST_ASSERT_EQUAL(50, calculateProgress(500, 1000));
}

void test_progress_hundred_percent(void) {
    TEST_ASSERT_EQUAL(100, calculateProgress(1000, 1000));
}

void test_progress_zero_file_size(void) {
    // Should handle division by zero gracefully
    TEST_ASSERT_EQUAL(0, calculateProgress(0, 0));
}

void test_progress_small_file(void) {
    TEST_ASSERT_EQUAL(50, calculateProgress(50, 100));
}

void test_progress_large_file(void) {
    // 1GB file, 256MB read
    size_t fileSize = 1024UL * 1024 * 1024;
    size_t bytesRead = 256UL * 1024 * 1024;
    TEST_ASSERT_EQUAL(25, calculateProgress(bytesRead, fileSize));
}

void test_progress_one_percent(void) {
    TEST_ASSERT_EQUAL(1, calculateProgress(10, 1000));
}

void test_progress_ninety_nine_percent(void) {
    TEST_ASSERT_EQUAL(99, calculateProgress(990, 1000));
}

void test_progress_rounding_down(void) {
    // 333/1000 = 33.3%, should be 33
    TEST_ASSERT_EQUAL(33, calculateProgress(333, 1000));
}

void test_progress_over_hundred(void) {
    // Edge case: bytesRead > fileSize (shouldn't happen, but should handle)
    // This might return > 100, which is acceptable for this implementation
    int result = calculateProgress(1100, 1000);
    TEST_ASSERT_GREATER_OR_EQUAL(100, result);
}

// ============================================================================
// Test: calculateETA()
// ============================================================================

void test_eta_at_zero_percent(void) {
    // At 0%, can't calculate ETA
    TEST_ASSERT_EQUAL(0, calculateETA(0, 5000));
}

void test_eta_at_fifty_percent(void) {
    // At 50% with 60 seconds elapsed, remaining should be ~60 seconds
    unsigned long eta = calculateETA(50, 60000);
    TEST_ASSERT_EQUAL(60000, eta);
}

void test_eta_at_hundred_percent(void) {
    // Job complete, no remaining time
    TEST_ASSERT_EQUAL(0, calculateETA(100, 60000));
}

void test_eta_at_twenty_five_percent(void) {
    // At 25% with 30 seconds elapsed, remaining should be 90 seconds
    // Formula: (elapsed * (100 - pct)) / pct = (30000 * 75) / 25 = 90000
    unsigned long eta = calculateETA(25, 30000);
    TEST_ASSERT_EQUAL(90000, eta);
}

void test_eta_at_seventy_five_percent(void) {
    // At 75% with 90 seconds elapsed, remaining should be 30 seconds
    // Formula: (90000 * 25) / 75 = 30000
    unsigned long eta = calculateETA(75, 90000);
    TEST_ASSERT_EQUAL(30000, eta);
}

void test_eta_at_one_percent(void) {
    // At 1% with 1 second elapsed, remaining should be 99 seconds
    unsigned long eta = calculateETA(1, 1000);
    TEST_ASSERT_EQUAL(99000, eta);
}

void test_eta_at_ninety_nine_percent(void) {
    // At 99% with 99 seconds elapsed, remaining should be ~1 second
    unsigned long eta = calculateETA(99, 99000);
    TEST_ASSERT_EQUAL(1000, eta);
}

void test_eta_with_large_elapsed(void) {
    // At 10% with 10 minutes elapsed
    unsigned long tenMinutes = 10 * 60 * 1000;  // 600000 ms
    unsigned long eta = calculateETA(10, tenMinutes);
    // (600000 * 90) / 10 = 5400000 ms = 90 minutes
    TEST_ASSERT_EQUAL(5400000UL, eta);
}

void test_eta_negative_not_possible(void) {
    // With valid input, ETA should never be negative
    // Since we use unsigned long, negative isn't possible by type
    // But test that we don't overflow
    unsigned long eta = calculateETA(50, 60000);
    TEST_ASSERT_NOT_EQUAL(0xFFFFFFFF, eta);  // Not max uint
}

// ============================================================================
// Combined Progress and ETA Tests
// ============================================================================

void test_progress_and_eta_consistency(void) {
    // Simulate a job at different stages
    size_t fileSize = 10000;
    unsigned long startTime = 0;

    // At 20%
    int pct20 = calculateProgress(2000, fileSize);
    TEST_ASSERT_EQUAL(20, pct20);

    // Mock: 20% took 10 seconds, so ETA should be 40 seconds
    unsigned long eta20 = calculateETA(20, 10000);
    TEST_ASSERT_EQUAL(40000, eta20);  // 40 seconds

    // At 80%
    int pct80 = calculateProgress(8000, fileSize);
    TEST_ASSERT_EQUAL(80, pct80);

    // Mock: 80% took 40 seconds, so ETA should be 10 seconds
    unsigned long eta80 = calculateETA(80, 40000);
    TEST_ASSERT_EQUAL(10000, eta80);  // 10 seconds
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

    // calculateProgress tests
    RUN_TEST(test_progress_zero_percent);
    RUN_TEST(test_progress_fifty_percent);
    RUN_TEST(test_progress_hundred_percent);
    RUN_TEST(test_progress_zero_file_size);
    RUN_TEST(test_progress_small_file);
    RUN_TEST(test_progress_large_file);
    RUN_TEST(test_progress_one_percent);
    RUN_TEST(test_progress_ninety_nine_percent);
    RUN_TEST(test_progress_rounding_down);
    RUN_TEST(test_progress_over_hundred);

    // calculateETA tests
    RUN_TEST(test_eta_at_zero_percent);
    RUN_TEST(test_eta_at_fifty_percent);
    RUN_TEST(test_eta_at_hundred_percent);
    RUN_TEST(test_eta_at_twenty_five_percent);
    RUN_TEST(test_eta_at_seventy_five_percent);
    RUN_TEST(test_eta_at_one_percent);
    RUN_TEST(test_eta_at_ninety_nine_percent);
    RUN_TEST(test_eta_with_large_elapsed);
    RUN_TEST(test_eta_negative_not_possible);

    // Combined tests
    RUN_TEST(test_progress_and_eta_consistency);

    return UNITY_END();
}

