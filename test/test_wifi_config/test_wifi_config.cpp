/**
 * Unit Tests: WiFi Configuration Parsing
 *
 * Tests for parsing WiFi credentials from configuration files.
 */

#include <unity.h>
#include "testable_logic.h"
#include <cstring>

// ============================================================================
// Test: parseWiFiConfig() - Valid Configurations
// ============================================================================

void test_wifi_config_basic(void) {
    const char* config = "SSID=MyNetwork\nPASS=MyPassword";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("MyNetwork", ssid);
    TEST_ASSERT_EQUAL_STRING("MyPassword", pass);
}

void test_wifi_config_with_spaces_around_equals(void) {
    const char* config = "SSID = MyNetwork\nPASS = MyPassword";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("MyNetwork", ssid);
    TEST_ASSERT_EQUAL_STRING("MyPassword", pass);
}

void test_wifi_config_with_comments(void) {
    const char* config = "# WiFi Configuration\n"
                          "# Network name\n"
                          "SSID=HomeNetwork\n"
                          "# Password\n"
                          "PASS=secret123";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("HomeNetwork", ssid);
    TEST_ASSERT_EQUAL_STRING("secret123", pass);
}

void test_wifi_config_empty_password(void) {
    const char* config = "SSID=OpenNetwork\nPASS=";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("OpenNetwork", ssid);
    TEST_ASSERT_EQUAL_STRING("", pass);
}

void test_wifi_config_password_with_spaces(void) {
    const char* config = "SSID=TestNet\nPASS=My Secret Password";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("TestNet", ssid);
    TEST_ASSERT_EQUAL_STRING("My Secret Password", pass);
}

void test_wifi_config_ssid_with_numbers(void) {
    const char* config = "SSID=Network123\nPASS=pass456";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("Network123", ssid);
    TEST_ASSERT_EQUAL_STRING("pass456", pass);
}

void test_wifi_config_special_chars_in_password(void) {
    const char* config = "SSID=SecureNet\nPASS=P@$$w0rd!#%";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("SecureNet", ssid);
    TEST_ASSERT_EQUAL_STRING("P@$$w0rd!#%", pass);
}

void test_wifi_config_empty_lines(void) {
    const char* config = "\n\nSSID=Net\n\nPASS=pw\n\n";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("Net", ssid);
    TEST_ASSERT_EQUAL_STRING("pw", pass);
}

void test_wifi_config_windows_line_endings(void) {
    const char* config = "SSID=WinNet\r\nPASS=WinPass\r\n";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("WinNet", ssid);
    TEST_ASSERT_EQUAL_STRING("WinPass", pass);
}

void test_wifi_config_reverse_order(void) {
    // Password before SSID - should still work
    const char* config = "PASS=secret\nSSID=Network";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("Network", ssid);
    TEST_ASSERT_EQUAL_STRING("secret", pass);
}

void test_wifi_config_ssid_only(void) {
    const char* config = "SSID=OnlySSID";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("OnlySSID", ssid);
    TEST_ASSERT_EQUAL_STRING("", pass);  // Password not set
}

void test_wifi_config_leading_spaces_in_value(void) {
    const char* config = "SSID=  LeadingSpaces\nPASS=  password";
    char ssid[64], pass[64];

    // Current implementation should trim leading spaces
    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("LeadingSpaces", ssid);
    TEST_ASSERT_EQUAL_STRING("password", pass);
}

void test_wifi_config_trailing_spaces_in_value(void) {
    const char* config = "SSID=TrailingSpaces  \nPASS=password  ";
    char ssid[64], pass[64];

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("TrailingSpaces", ssid);
    TEST_ASSERT_EQUAL_STRING("password", pass);
}

// ============================================================================
// Test: parseWiFiConfig() - Invalid Configurations
// ============================================================================

void test_wifi_config_missing_ssid(void) {
    const char* config = "PASS=password";
    char ssid[64], pass[64];

    // Should fail - SSID is required
    TEST_ASSERT_FALSE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
}

void test_wifi_config_empty_string(void) {
    const char* config = "";
    char ssid[64], pass[64];

    TEST_ASSERT_FALSE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
}

void test_wifi_config_only_comments(void) {
    const char* config = "# This is a comment\n# Another comment";
    char ssid[64], pass[64];

    TEST_ASSERT_FALSE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
}

void test_wifi_config_null_input(void) {
    char ssid[64], pass[64];

    TEST_ASSERT_FALSE(parseWiFiConfig(nullptr, ssid, sizeof(ssid), pass, sizeof(pass)));
}

void test_wifi_config_null_output_ssid(void) {
    const char* config = "SSID=Net\nPASS=pw";
    char pass[64];

    TEST_ASSERT_FALSE(parseWiFiConfig(config, nullptr, 0, pass, sizeof(pass)));
}

void test_wifi_config_null_output_pass(void) {
    const char* config = "SSID=Net\nPASS=pw";
    char ssid[64];

    TEST_ASSERT_FALSE(parseWiFiConfig(config, ssid, sizeof(ssid), nullptr, 0));
}

void test_wifi_config_no_equals_sign(void) {
    const char* config = "SSID MyNetwork\nPASS MyPassword";
    char ssid[64], pass[64];

    // Lines without '=' should be skipped
    TEST_ASSERT_FALSE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
}

void test_wifi_config_unknown_keys(void) {
    const char* config = "NAME=TestNet\nKEY=secret";
    char ssid[64], pass[64];

    // Unknown keys should be ignored, and SSID is missing
    TEST_ASSERT_FALSE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
}

// ============================================================================
// Test: parseWiFiConfig() - Buffer Size Edge Cases
// ============================================================================

void test_wifi_config_ssid_truncated(void) {
    const char* config = "SSID=VeryLongWiFiNetworkNameThatExceedsBufferSize\nPASS=pw";
    char ssid[10], pass[64];  // Small buffer

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    // Should be truncated to fit buffer
    TEST_ASSERT_TRUE(strlen(ssid) < 10);
    TEST_ASSERT_EQUAL_STRING("VeryLongW", ssid);  // 9 chars + null
}

void test_wifi_config_pass_truncated(void) {
    const char* config = "SSID=Net\nPASS=ThisIsAVeryLongPasswordThatExceedsBufferSize";
    char ssid[64], pass[10];  // Small buffer

    TEST_ASSERT_TRUE(parseWiFiConfig(config, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_TRUE(strlen(pass) < 10);
    TEST_ASSERT_EQUAL_STRING("ThisIsAVe", pass);  // 9 chars + null
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

    // Valid configuration tests
    RUN_TEST(test_wifi_config_basic);
    RUN_TEST(test_wifi_config_with_spaces_around_equals);
    RUN_TEST(test_wifi_config_with_comments);
    RUN_TEST(test_wifi_config_empty_password);
    RUN_TEST(test_wifi_config_password_with_spaces);
    RUN_TEST(test_wifi_config_ssid_with_numbers);
    RUN_TEST(test_wifi_config_special_chars_in_password);
    RUN_TEST(test_wifi_config_empty_lines);
    RUN_TEST(test_wifi_config_windows_line_endings);
    RUN_TEST(test_wifi_config_reverse_order);
    RUN_TEST(test_wifi_config_ssid_only);
    RUN_TEST(test_wifi_config_leading_spaces_in_value);
    RUN_TEST(test_wifi_config_trailing_spaces_in_value);

    // Invalid configuration tests
    RUN_TEST(test_wifi_config_missing_ssid);
    RUN_TEST(test_wifi_config_empty_string);
    RUN_TEST(test_wifi_config_only_comments);
    RUN_TEST(test_wifi_config_null_input);
    RUN_TEST(test_wifi_config_null_output_ssid);
    RUN_TEST(test_wifi_config_null_output_pass);
    RUN_TEST(test_wifi_config_no_equals_sign);
    RUN_TEST(test_wifi_config_unknown_keys);

    // Buffer size tests
    RUN_TEST(test_wifi_config_ssid_truncated);
    RUN_TEST(test_wifi_config_pass_truncated);

    return UNITY_END();
}

