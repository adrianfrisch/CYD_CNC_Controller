// =============================================================================
// Machine Config — Implementation
// =============================================================================

#include "machine_config.h"
#include "config.h"
#include <SPIFFS.h>

MachineConfigManager machineConfig;

void MachineConfigManager::begin() {
    if (loadConfig()) {
        Serial.printf("[CFG] Machine config loaded: homing=%s, clearance=%.1fmm\n",
                      _config.homingEnabled ? "yes" : "no",
                      _config.clearanceHeight);
    } else {
        Serial.println("[CFG] No machine.cfg found — using defaults");
    }
}

bool MachineConfigManager::loadConfig() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[CFG] SPIFFS mount failed");
        return false;
    }

    if (!SPIFFS.exists(MACHINE_CONFIG_FILE)) {
        return false;
    }

    File f = SPIFFS.open(MACHINE_CONFIG_FILE, "r");
    if (!f) {
        Serial.println("[CFG] Failed to open machine.cfg");
        return false;
    }

    // Read entire file into buffer
    size_t fileSize = f.size();
    if (fileSize == 0 || fileSize > 1024) {
        f.close();
        return false;
    }

    char buf[1025];
    size_t bytesRead = f.readBytes(buf, fileSize);
    buf[bytesRead] = '\0';
    f.close();

    return parseMachineConfig(buf, _config);
}

