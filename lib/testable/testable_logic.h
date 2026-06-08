#pragma once
/**
 * Testable Logic Module
 *
 * This module contains pure logic functions extracted from the embedded code
 * that can be unit tested without hardware dependencies.
 */

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

// ============================================================================
// GRBL State Enumeration
// ============================================================================

enum class GrblState : uint8_t {
    Unknown,
    Idle,
    Run,
    Hold,
    Jog,
    Alarm,
    Door,
    Check,
    Home,
    Sleep
};

// ============================================================================
// GRBL Status Structure
// ============================================================================

struct GrblStatus {
    GrblState state = GrblState::Unknown;
    float wposX = 0, wposY = 0, wposZ = 0;   // Work position
    float mposX = 0, mposY = 0, mposZ = 0;   // Machine position
    int feedRate = 0;
    int spindleSpeed = 0;
    int bufferAvail = 128;
    int rxAvail = 128;
    bool overrides = false;
    int feedOverride = 100;
    int rapidOverride = 100;
    int spindleOverride = 100;
};

// ============================================================================
// GRBL Parser Functions
// ============================================================================

/**
 * Parse a GRBL state string to enum
 * @param s State string (e.g., "Idle", "Run", "Hold:0")
 * @return Corresponding GrblState enum value
 */
GrblState parseGrblState(const char* s);

/**
 * Parse a full GRBL status report string
 * @param line Status report (e.g., "<Idle|WPos:0,0,0|Bf:15,128|FS:500,0>")
 * @param status Output status structure
 * @return true if parsing succeeded
 */
bool parseGrblStatus(const char* line, GrblStatus& status);

// ============================================================================
// GCode Processing Functions
// ============================================================================

/**
 * Strip comments and whitespace from a GCode line
 * @param line Input/output line buffer (modified in place)
 */
void stripGCodeComments(char* line);

/**
 * Check if a filename has a valid GCode extension
 * @param filename Filename to check
 * @return true if the extension is a valid GCode extension
 */
bool isGCodeExtension(const char* filename);

// ============================================================================
// Progress Calculation Functions
// ============================================================================

/**
 * Calculate job progress percentage
 * @param bytesRead Bytes read from file so far
 * @param fileSize Total file size in bytes
 * @return Percentage complete (0-100)
 */
int calculateProgress(size_t bytesRead, size_t fileSize);

/**
 * Calculate estimated remaining time
 * @param percentComplete Current progress percentage (1-100)
 * @param elapsedMs Time elapsed so far in milliseconds
 * @return Estimated remaining time in milliseconds
 */
unsigned long calculateETA(int percentComplete, unsigned long elapsedMs);

// ============================================================================
// WiFi Config Parsing Functions
// ============================================================================

/**
 * Parse WiFi configuration from a config string
 * @param configData Configuration data (key=value format)
 * @param ssidOut Output buffer for SSID
 * @param ssidMaxLen Maximum length of SSID buffer
 * @param passOut Output buffer for password
 * @param passMaxLen Maximum length of password buffer
 * @return true if at least SSID was parsed successfully
 */
bool parseWiFiConfig(const char* configData,
                     char* ssidOut, size_t ssidMaxLen,
                     char* passOut, size_t passMaxLen);

// ============================================================================
// String Utility Functions
// ============================================================================

/**
 * Case-insensitive string comparison
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal (ignoring case), non-zero otherwise
 */
int strcasecmp_portable(const char* s1, const char* s2);

// ============================================================================
// Machine Config Parsing Functions
// ============================================================================

/**
 * Machine configuration — features supported by the connected CNC
 */
struct MachineConfig {
    bool homingEnabled = true;   // Does the machine support $H homing?
};

/**
 * Parse machine configuration from a config string (key=value format)
 * @param configData Configuration data
 * @param config Output configuration structure
 * @return true if parsing succeeded (even if no keys matched — defaults are valid)
 */
bool parseMachineConfig(const char* configData, MachineConfig& config);

/**
 * Parse a boolean value string (yes/true/1 → true, no/false/0 → false)
 * @param val Value string
 * @param out Output boolean
 * @return true if the value was recognized
 */
bool parseBoolValue(const char* val, bool& out);
