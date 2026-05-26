/**
 * Unit Tests: SD Card File Extension Filtering
 *
 * Tests for checking if a filename has a valid GCode extension.
 */

#include <unity.h>
#include "testable_logic.h"

// ============================================================================
// Test: isGCodeExtension() - Valid Extensions
// ============================================================================

void test_extension_nc_lowercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("part.nc"));
}

void test_extension_nc_uppercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("PART.NC"));
}

void test_extension_nc_mixed_case(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("Part.Nc"));
}

void test_extension_gcode_lowercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("job.gcode"));
}

void test_extension_gcode_uppercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("JOB.GCODE"));
}

void test_extension_gc_lowercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("test.gc"));
}

void test_extension_ngc_lowercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("program.ngc"));
}

void test_extension_tap_lowercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("drill.tap"));
}

void test_extension_cnc_lowercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("router.cnc"));
}

void test_extension_txt_lowercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("notes.txt"));
}

void test_extension_txt_uppercase(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("README.TXT"));
}

// ============================================================================
// Test: isGCodeExtension() - Invalid Extensions
// ============================================================================

void test_extension_invalid_jpg(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("photo.jpg"));
}

void test_extension_invalid_exe(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("virus.exe"));
}

void test_extension_invalid_pdf(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("manual.pdf"));
}

void test_extension_invalid_mp3(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("song.mp3"));
}

void test_extension_invalid_doc(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("document.doc"));
}

void test_extension_invalid_zip(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("archive.zip"));
}

void test_extension_invalid_bin(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("firmware.bin"));
}

void test_extension_invalid_cpp(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("code.cpp"));
}

// ============================================================================
// Test: isGCodeExtension() - Edge Cases
// ============================================================================

void test_extension_no_extension(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("README"));
}

void test_extension_empty_string(void) {
    TEST_ASSERT_FALSE(isGCodeExtension(""));
}

void test_extension_null(void) {
    TEST_ASSERT_FALSE(isGCodeExtension(nullptr));
}

void test_extension_only_dot(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("."));
}

void test_extension_dot_at_start(void) {
    TEST_ASSERT_TRUE(isGCodeExtension(".hidden.nc"));
}

void test_extension_multiple_dots(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("my.part.v2.nc"));
}

void test_extension_long_filename(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("this_is_a_very_long_filename_for_testing_purposes.gcode"));
}

void test_extension_short_filename(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("a.nc"));
}

void test_extension_numbers_in_name(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("part123.nc"));
}

void test_extension_special_chars(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("my-part_v1.nc"));
}

void test_extension_spaces_in_name(void) {
    // Note: SD card FAT32 may not support spaces, but test the logic anyway
    TEST_ASSERT_TRUE(isGCodeExtension("my part.nc"));
}

void test_extension_similar_but_invalid_nc2(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("file.nc2"));
}

void test_extension_similar_but_invalid_ncc(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("file.ncc"));
}

void test_extension_partial_match_gcode2(void) {
    TEST_ASSERT_FALSE(isGCodeExtension("file.gcode2"));
}

void test_extension_case_sensitivity_mix(void) {
    TEST_ASSERT_TRUE(isGCodeExtension("MixedCase.GcOdE"));
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

    // Valid extension tests
    RUN_TEST(test_extension_nc_lowercase);
    RUN_TEST(test_extension_nc_uppercase);
    RUN_TEST(test_extension_nc_mixed_case);
    RUN_TEST(test_extension_gcode_lowercase);
    RUN_TEST(test_extension_gcode_uppercase);
    RUN_TEST(test_extension_gc_lowercase);
    RUN_TEST(test_extension_ngc_lowercase);
    RUN_TEST(test_extension_tap_lowercase);
    RUN_TEST(test_extension_cnc_lowercase);
    RUN_TEST(test_extension_txt_lowercase);
    RUN_TEST(test_extension_txt_uppercase);

    // Invalid extension tests
    RUN_TEST(test_extension_invalid_jpg);
    RUN_TEST(test_extension_invalid_exe);
    RUN_TEST(test_extension_invalid_pdf);
    RUN_TEST(test_extension_invalid_mp3);
    RUN_TEST(test_extension_invalid_doc);
    RUN_TEST(test_extension_invalid_zip);
    RUN_TEST(test_extension_invalid_bin);
    RUN_TEST(test_extension_invalid_cpp);

    // Edge case tests
    RUN_TEST(test_extension_no_extension);
    RUN_TEST(test_extension_empty_string);
    RUN_TEST(test_extension_null);
    RUN_TEST(test_extension_only_dot);
    RUN_TEST(test_extension_dot_at_start);
    RUN_TEST(test_extension_multiple_dots);
    RUN_TEST(test_extension_long_filename);
    RUN_TEST(test_extension_short_filename);
    RUN_TEST(test_extension_numbers_in_name);
    RUN_TEST(test_extension_special_chars);
    RUN_TEST(test_extension_spaces_in_name);
    RUN_TEST(test_extension_similar_but_invalid_nc2);
    RUN_TEST(test_extension_similar_but_invalid_ncc);
    RUN_TEST(test_extension_partial_match_gcode2);
    RUN_TEST(test_extension_case_sensitivity_mix);

    return UNITY_END();
}

