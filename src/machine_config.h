#pragma once
// =============================================================================
// Machine Config — loads machine feature flags from /machine.cfg on SPIFFS
// =============================================================================

#include <Arduino.h>
#include "../lib/testable/testable_logic.h"

#define MACHINE_CONFIG_FILE  "/machine.cfg"

class MachineConfigManager {
public:
    void begin();

    bool homingEnabled() const { return _config.homingEnabled; }
    const MachineConfig& config() const { return _config; }

private:
    MachineConfig _config;
    bool loadConfig();
};

extern MachineConfigManager machineConfig;

