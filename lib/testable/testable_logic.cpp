/**
 * Testable Logic Module — Implementation
 *
 * Pure logic functions that can be unit tested without hardware dependencies.
 */

#include "testable_logic.h"
#include <cctype>
#include <cstdlib>

// ============================================================================
// String Utilities
// ============================================================================

int strcasecmp_portable(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

// ============================================================================
// GRBL Parser Functions
// ============================================================================

GrblState parseGrblState(const char* s) {
    if (!s) return GrblState::Unknown;

    if (strcmp(s, "Idle")  == 0) return GrblState::Idle;
    if (strcmp(s, "Run")   == 0) return GrblState::Run;
    if (strcmp(s, "Hold")  == 0 || strncmp(s, "Hold:", 5) == 0) return GrblState::Hold;
    if (strcmp(s, "Jog")   == 0) return GrblState::Jog;
    if (strcmp(s, "Alarm") == 0) return GrblState::Alarm;
    if (strcmp(s, "Door")  == 0 || strncmp(s, "Door:", 5) == 0) return GrblState::Door;
    if (strcmp(s, "Check") == 0) return GrblState::Check;
    if (strcmp(s, "Home")  == 0) return GrblState::Home;
    if (strcmp(s, "Sleep") == 0) return GrblState::Sleep;

    return GrblState::Unknown;
}

bool parseGrblStatus(const char* line, GrblStatus& status) {
    if (!line || line[0] != '<') return false;

    // Work with a copy to avoid modifying input
    char buf[256];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // Remove < and >
    char* p = buf;
    if (*p == '<') p++;
    char* end = strrchr(p, '>');
    if (end) *end = '\0';

    // First token is state
    char* tok = strtok(p, "|");
    if (tok) {
        status.state = parseGrblState(tok);
    } else {
        return false;
    }

    // Remaining tokens
    while ((tok = strtok(nullptr, "|")) != nullptr) {
        if (strncmp(tok, "WPos:", 5) == 0) {
            sscanf(tok + 5, "%f,%f,%f", &status.wposX, &status.wposY, &status.wposZ);
        } else if (strncmp(tok, "MPos:", 5) == 0) {
            sscanf(tok + 5, "%f,%f,%f", &status.mposX, &status.mposY, &status.mposZ);
            // If WPos not reported, copy MPos
            status.wposX = status.mposX;
            status.wposY = status.mposY;
            status.wposZ = status.mposZ;
        } else if (strncmp(tok, "Bf:", 3) == 0) {
            sscanf(tok + 3, "%d,%d", &status.bufferAvail, &status.rxAvail);
        } else if (strncmp(tok, "FS:", 3) == 0) {
            sscanf(tok + 3, "%d,%d", &status.feedRate, &status.spindleSpeed);
        } else if (strncmp(tok, "F:", 2) == 0) {
            sscanf(tok + 2, "%d", &status.feedRate);
        } else if (strncmp(tok, "Ov:", 3) == 0) {
            sscanf(tok + 3, "%d,%d,%d", &status.feedOverride,
                   &status.rapidOverride, &status.spindleOverride);
            status.overrides = true;
        }
    }

    return true;
}

// ============================================================================
// GCode Processing Functions
// ============================================================================

void stripGCodeComments(char* line) {
    if (!line) return;

    // Remove parenthetical comments: (...)
    char* p = strchr(line, '(');
    if (p) *p = '\0';

    // Remove semicolon comments: ;...
    p = strchr(line, ';');
    if (p) *p = '\0';

    // Trim trailing whitespace
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }

    // Trim leading whitespace
    p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (p != line) memmove(line, p, strlen(p) + 1);
}

bool isGCodeExtension(const char* filename) {
    if (!filename) return false;

    size_t len = strlen(filename);
    if (len < 3) return false;  // Need at least ".x" extension

    // Find the extension
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return false;

    const char* ext = dot + 1;

    // Check against valid extensions
    if (strcasecmp_portable(ext, "nc") == 0) return true;
    if (strcasecmp_portable(ext, "gc") == 0) return true;
    if (strcasecmp_portable(ext, "gcode") == 0) return true;
    if (strcasecmp_portable(ext, "ngc") == 0) return true;
    if (strcasecmp_portable(ext, "tap") == 0) return true;
    if (strcasecmp_portable(ext, "cnc") == 0) return true;
    if (strcasecmp_portable(ext, "txt") == 0) return true;

    return false;
}

// ============================================================================
// Progress Calculation Functions
// ============================================================================

int calculateProgress(size_t bytesRead, size_t fileSize) {
    if (fileSize == 0) return 0;
    return (int)((bytesRead * 100UL) / fileSize);
}

unsigned long calculateETA(int percentComplete, unsigned long elapsedMs) {
    if (percentComplete <= 0) return 0;
    if (percentComplete >= 100) return 0;
    return (elapsedMs * (100 - percentComplete)) / percentComplete;
}

// ============================================================================
// WiFi Config Parsing Functions
// ============================================================================

bool parseWiFiConfig(const char* configData,
                     char* ssidOut, size_t ssidMaxLen,
                     char* passOut, size_t passMaxLen) {
    if (!configData || !ssidOut || !passOut) return false;

    // Initialize outputs
    ssidOut[0] = '\0';
    passOut[0] = '\0';

    // Work with a copy
    size_t dataLen = strlen(configData);
    char* buf = new char[dataLen + 1];
    strcpy(buf, configData);

    // Process line by line
    char* line = strtok(buf, "\n");
    while (line) {
        // Trim leading whitespace
        while (*line == ' ' || *line == '\t') line++;

        // Skip comments and empty lines
        if (*line == '#' || *line == '\0' || *line == '\r') {
            line = strtok(nullptr, "\n");
            continue;
        }

        // Find '=' separator
        char* eq = strchr(line, '=');
        if (!eq) {
            line = strtok(nullptr, "\n");
            continue;
        }

        // Split key and value
        *eq = '\0';
        char* key = line;
        char* val = eq + 1;

        // Trim whitespace from key
        while (*key == ' ' || *key == '\t') key++;
        char* keyEnd = eq - 1;
        while (keyEnd > key && (*keyEnd == ' ' || *keyEnd == '\t')) {
            *keyEnd = '\0';
            keyEnd--;
        }

        // Trim whitespace from value
        while (*val == ' ' || *val == '\t') val++;
        size_t valLen = strlen(val);
        while (valLen > 0 && (val[valLen-1] == ' ' || val[valLen-1] == '\t' || val[valLen-1] == '\r')) {
            val[--valLen] = '\0';
        }

        // Match key
        if (strcmp(key, "SSID") == 0) {
            strncpy(ssidOut, val, ssidMaxLen - 1);
            ssidOut[ssidMaxLen - 1] = '\0';
        } else if (strcmp(key, "PASS") == 0) {
            strncpy(passOut, val, passMaxLen - 1);
            passOut[passMaxLen - 1] = '\0';
        }

        line = strtok(nullptr, "\n");
    }

    delete[] buf;

    // Return true if at least SSID was found
    return ssidOut[0] != '\0';
}

