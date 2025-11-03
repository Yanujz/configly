#include <configly/configly.hpp>
#include <iostream>

// ===================================================================
// 1. Define the Configuration Struct
// ===================================================================
// The struct must be trivially copyable.
struct AppSettings {
    int   speed;
    bool  enabled;
    float calibration_factor;
};

// ===================================================================
// 2. Implement Persistence and Callback Functions
// ===================================================================
// These must be free functions or static class members.

bool saveSettings(const AppSettings& cfg) {
    std::cout << "[PERSISTENCE] ==> Saving settings..." << std::endl;
    std::cout << "    - Speed: " << cfg.speed << std::endl;
    std::cout << "    - Enabled: " << (cfg.enabled ? "true" : "false") << std::endl;
    std::cout << "    - Calibration Factor: " << cfg.calibration_factor << std::endl;
    return true; // Return true for success
}

bool loadSettings(AppSettings& cfg) {
    std::cout << "[PERSISTENCE] <== Loading settings..." << std::endl;
    cfg.speed = 9999;
    cfg.enabled = true;
    cfg.calibration_factor = 3.14f;
    return true; // Return true for success
}

void onSpeedChange(const int& newSpeed, void* userData) {
    std::cout << "[CALLBACK] Speed changed to: " << newSpeed << std::endl;
    if (userData) {
        int* counter = static_cast<int*>(userData);
        (*counter)++;
    }
}

void onEnabledChange(const bool& isEnabled, void* userData) {
    std::cout << "[CALLBACK] Enabled state changed to: " << (isEnabled ? "true" : "false") << std::endl;
    if (userData) {
        int* counter = static_cast<int*>(userData);
        (*counter)++;
    }
}

// ===================================================================
// 3. Main Application Logic
// ===================================================================
int main() {
    std::cout << "--- Configly Advanced Usage Example ---" << std::endl;

    // Get the singleton instance for our settings struct
    auto& settings = Configly<AppSettings>::instance();

    // --- SETUP PHASE ---
    std::cout << "\n--- 1. Initial Setup ---" << std::endl;

    settings.setDefault({ 100, false, 1.0f });
    settings.setSaveFunction(&saveSettings);
    settings.setLoadFunction(&loadSettings);

    // Register callbacks - no handles returned (one callback per field)
    // Method chaining is supported for cleaner syntax
    int callbackExecutionCounter = 0;
    settings.onChange(&AppSettings::speed, &onSpeedChange, &callbackExecutionCounter)
            .onChange(&AppSettings::enabled, &onEnabledChange, &callbackExecutionCounter);

    std::cout << "Initial speed: " << settings.get(&AppSettings::speed) << std::endl;
    std::cout << "Initial enabled state: " << (settings.get(&AppSettings::enabled) ? "true" : "false") << std::endl;

    // --- DEMONSTRATION PHASE ---
    
    std::cout << "\n--- 2. Demonstrating load() ---" << std::endl;
    if (settings.load()) {
        std::cout << "Settings loaded successfully." << std::endl;
    } else {
        std::cout << "Failed to load settings." << std::endl;
    }
    
    std::cout << "Speed after load: " << settings.get(&AppSettings::speed) << std::endl;
    std::cout << "Enabled state after load: " << (settings.get(&AppSettings::enabled) ? "true" : "false") << std::endl;

    std::cout << "\n--- 3. Demonstrating set() ---" << std::endl;
    settings.set(&AppSettings::speed, 500);

    std::cout << "\n--- 4. Demonstrating save() ---" << std::endl;
    if (settings.save()) {
        std::cout << "Settings saved successfully." << std::endl;
    } else {
        std::cout << "Failed to save settings." << std::endl;
    }
    
    std::cout << "\n--- 5. Demonstrating restoreDefaults() ---" << std::endl;
    settings.restoreDefaults();
    std::cout << "Speed after restore: " << settings.get(&AppSettings::speed) << std::endl;
    
    std::cout << "\n--- 6. Demonstrating restoreDefault() for single field ---" << std::endl;
    settings.set(&AppSettings::speed, 777);
    std::cout << "Speed set to: " << settings.get(&AppSettings::speed) << std::endl;
    settings.restoreDefault(&AppSettings::speed);
    std::cout << "Speed after restoreDefault: " << settings.get(&AppSettings::speed) << std::endl;
    
    std::cout << "\n--- 7. Demonstrating callback removal ---" << std::endl;
    settings.removeCallback(&AppSettings::enabled);
    settings.set(&AppSettings::enabled, false); // This won't trigger callback anymore
    std::cout << "Enabled changed to false (no callback triggered)" << std::endl;
    
    std::cout << "\n--- 8. Demonstrating getAll() ---" << std::endl;
    AppSettings snapshot;
    settings.getAll(snapshot);
    std::cout << "Snapshot retrieved:" << std::endl;
    std::cout << "    - Speed: " << snapshot.speed << std::endl;
    std::cout << "    - Enabled: " << (snapshot.enabled ? "true" : "false") << std::endl;
    std::cout << "    - Calibration Factor: " << snapshot.calibration_factor << std::endl;
    
    std::cout << "\n--- SUMMARY ---" << std::endl;
    std::cout << "Total callback executions: " << callbackExecutionCounter << std::endl;

    return 0;
}