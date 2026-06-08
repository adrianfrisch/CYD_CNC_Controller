/**
 * Unit Tests: Machine Configuration Parsing
 *
 * Tests for parsing machine feature flags from configuration files.
 */

#include <unity.h>
#include "testable_logic.h"
#include <cstring>

// ============================================================================
// Test: parseBoolValue()
// ============================================================================

void test_bool_value_yes(void) {
    bool out = false;
    TEST_ASSERT_TRUE(parseBoolValue("yes", out));
    TEST_ASSERT_TRUE(out);
}

void test_bool_value_true(void) {
    bool out = false;
    TEST_ASSERT_TRUE(parseBoolValue("true", out));
    TEST_ASSERT_TRUE(out);
}

void test_bool_value_1(void) {
    bool out = false;
    TEST_ASSERT_TRUE(parseBoolValue("1", out));
    TEST_ASSERT_TRUE(out);
}

void test_bool_value_no(void) {
    bool out = true;
    TEST_ASSERT_TRUE(parseBoolValue("no", out));
    TEST_ASSERT_FALSE(out);
}

void test_bool_value_false(void) {
    bool out = true;
    TEST_ASSERT_TRUE(parseBoolValue("false", out));
    TEST_ASSERT_FALSE(out);
}

void test_bool_value_0(void) {
    bool out = true;
    TEST_ASSERT_TRUE(parseBoolValue("0", out));
    TEST_ASSERT_FALSE(out);
}

void test_bool_value_case_insensitive(void) {
    bool out = false;
    TEST_ASSERT_TRUE(parseBoolValue("YES", out));
    TEST_ASSERT_TRUE(out);
    TEST_ASSERT_TRUE(parseBoolValue("True", out));
    TEST_ASSERT_TRUE(out);
    TEST_ASSERT_TRUE(parseBoolValue("NO", out));
    TEST_ASSERT_FALSE(out);
    TEST_ASSERT_TRUE(parseBoolValue("False", out));
    TEST_ASSERT_FALSE(out);
}

void test_bool_value_invalid(void) {
    bool out = true;
    TEST_ASSERT_FALSE(parseBoolValue("maybe", out));
    TEST_ASSERT_TRUE(out); // unchanged
    TEST_ASSERT_FALSE(parseBoolValue("", out));
    TEST_ASSERT_FALSE(parseBoolValue(nullptr, out));
}

// ============================================================================
// Test: parseMachineConfig() — Homing
// ============================================================================

void test_machine_config_homing_yes(void) {
    const char* config = "HOMING=yes";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_TRUE(cfg.homingEnabled);
}

void test_machine_config_homing_no(void) {
    const char* config = "HOMING=no";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FALSE(cfg.homingEnabled);
}

void test_machine_config_homing_true(void) {
    const char* config = "HOMING=true";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_TRUE(cfg.homingEnabled);
}

void test_machine_config_homing_false(void) {
    const char* config = "HOMING=false";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FALSE(cfg.homingEnabled);
}

void test_machine_config_homing_1(void) {
    const char* config = "HOMING=1";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_TRUE(cfg.homingEnabled);
}

void test_machine_config_homing_0(void) {
    const char* config = "HOMING=0";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FALSE(cfg.homingEnabled);
}

// ============================================================================
// Test: parseMachineConfig() — Defaults
// ============================================================================

void test_machine_config_defaults(void) {
    const char* config = "# empty config with only comments\n";
    MachineConfig cfg;
    cfg.homingEnabled = false; // set non-default
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_TRUE(cfg.homingEnabled); // default is true
}

void test_machine_config_empty_string(void) {
    const char* config = "";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_TRUE(cfg.homingEnabled); // default
}

void test_machine_config_null(void) {
    MachineConfig cfg;
    TEST_ASSERT_FALSE(parseMachineConfig(nullptr, cfg));
}

// ============================================================================
// Test: parseMachineConfig() — Comments & whitespace
// ============================================================================

void test_machine_config_with_comments(void) {
    const char* config = "# Machine config\n"
                          "# Homing support\n"
                          "HOMING=no\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FALSE(cfg.homingEnabled);
}

void test_machine_config_with_spaces(void) {
    const char* config = "  HOMING = no  \n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FALSE(cfg.homingEnabled);
}

void test_machine_config_case_insensitive_key(void) {
    const char* config = "homing=no";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FALSE(cfg.homingEnabled);
}

void test_machine_config_unknown_key_ignored(void) {
    const char* config = "UNKNOWN=something\nHOMING=no\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FALSE(cfg.homingEnabled);
}

void test_machine_config_full_example(void) {
    const char* config = "# CYD CNC Controller — Machine Configuration\n"
                          "# ==============================================\n"
                          "\n"
                          "# Does the machine support homing ($H)?\n"
                          "HOMING=yes\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_TRUE(cfg.homingEnabled);
}

// ============================================================================
// Test: parseMachineConfig() — Clearance Height
// ============================================================================

void test_machine_config_clearance_height_default(void) {
    const char* config = "";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cfg.clearanceHeight);
}

void test_machine_config_clearance_height_set(void) {
    const char* config = "CLEARANCE_HEIGHT=10.0\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, cfg.clearanceHeight);
}

void test_machine_config_clearance_height_integer(void) {
    const char* config = "CLEARANCE_HEIGHT=5\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, cfg.clearanceHeight);
}

void test_machine_config_clearance_height_zero_disabled(void) {
    const char* config = "CLEARANCE_HEIGHT=0\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cfg.clearanceHeight);
}

void test_machine_config_clearance_height_with_homing(void) {
    const char* config = "HOMING=yes\nCLEARANCE_HEIGHT=15.5\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_TRUE(cfg.homingEnabled);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.5f, cfg.clearanceHeight);
}

void test_machine_config_clearance_height_case_insensitive(void) {
    const char* config = "clearance_height=20\n";
    MachineConfig cfg;
    TEST_ASSERT_TRUE(parseMachineConfig(config, cfg));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, cfg.clearanceHeight);
}

// ============================================================================
// Test Runner
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // parseBoolValue tests
    RUN_TEST(test_bool_value_yes);
    RUN_TEST(test_bool_value_true);
    RUN_TEST(test_bool_value_1);
    RUN_TEST(test_bool_value_no);
    RUN_TEST(test_bool_value_false);
    RUN_TEST(test_bool_value_0);
    RUN_TEST(test_bool_value_case_insensitive);
    RUN_TEST(test_bool_value_invalid);

    // parseMachineConfig tests — homing
    RUN_TEST(test_machine_config_homing_yes);
    RUN_TEST(test_machine_config_homing_no);
    RUN_TEST(test_machine_config_homing_true);
    RUN_TEST(test_machine_config_homing_false);
    RUN_TEST(test_machine_config_homing_1);
    RUN_TEST(test_machine_config_homing_0);

    // parseMachineConfig tests — defaults & edge cases
    RUN_TEST(test_machine_config_defaults);
    RUN_TEST(test_machine_config_empty_string);
    RUN_TEST(test_machine_config_null);

    // parseMachineConfig tests — comments & whitespace
    RUN_TEST(test_machine_config_with_comments);
    RUN_TEST(test_machine_config_with_spaces);
    RUN_TEST(test_machine_config_case_insensitive_key);
    RUN_TEST(test_machine_config_unknown_key_ignored);
    RUN_TEST(test_machine_config_full_example);

    // parseMachineConfig tests — clearance height
    RUN_TEST(test_machine_config_clearance_height_default);
    RUN_TEST(test_machine_config_clearance_height_set);
    RUN_TEST(test_machine_config_clearance_height_integer);
    RUN_TEST(test_machine_config_clearance_height_zero_disabled);
    RUN_TEST(test_machine_config_clearance_height_with_homing);
    RUN_TEST(test_machine_config_clearance_height_case_insensitive);

    return UNITY_END();
}

