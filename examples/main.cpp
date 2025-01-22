#include <iostream>
#include "configly.hpp"

// Define the configuration structure that holds settings
struct Config
{
    int   volume;
    float brightness;
};

// Default configuration values
Config defaultConfig = { 50, 0.75f }; // Default volume is 50, default brightness is 0.75
Config userConfig;                    // User-defined configuration (initially empty)

int main()
{
    // Set the default configuration values for the application
    // Configly will initialize with these default settings
    Configly<Config>::instance().setDefault(defaultConfig);

    // Set custom save function. This function will be called when saving configuration.
    // Replace the lambda body with your actual save logic.
    Configly<Config>::instance().setSaveFunction([](const Config &cfg) -> bool {
        // Your logic to save the configuration to disk or persistent storage
        std::cout << "Saving configuration..." << std::endl;
        return false;  // Return 'false' to indicate save failure or 'true' if successful
    });

    // Set custom load function. This function will be called when loading the configuration.
    // Replace the lambda body with your actual load logic.
    Configly<Config>::instance().setLoadFunction([](Config &cfg) -> bool {
        // Your logic to load the configuration from persistent storage
        std::cout << "Loading configuration..." << std::endl;
        return false;  // Return 'false' to indicate load failure or 'true' if successful
    });


    int currentVolume = -1;

    if(Configly<Config>::instance().get(&Config::volume, currentVolume))
    {
        std::cout << "Current volume before set: " << currentVolume << std::endl;
    }
    else
    {
        std::cout << "Fail to get volume value" << std::endl;
    }

    // Modify the 'volume' configuration parameter using the 'set()' function
    // This will set the volume to 80 in the user configuration
    Configly<Config>::instance().set(&Config::volume, 80);
    if(Configly<Config>::instance().get(&Config::volume, currentVolume))
    {
        std::cout << "Current volume after set: " << currentVolume << std::endl;
    }
    else
    {
        std::cout << "Fail to get volume value" << std::endl;
    }
    
    std::cout << "------------------------" << std::endl;
    
    // Register callback functions to handle changes to the 'volume' and 'brightness' parameters.
    // These functions will be called when the respective parameters are changed.
    Configly<Config>::instance()
    .onChange(&Config::volume, [](const int &newVolume) -> void {
        // Callback for volume changes
        std::cout << "[CB] Volume changed to: " << newVolume << std::endl;
    }).onChange(&Config::brightness, [](const float &brightness) -> void {
        // Callback for brightness changes
        std::cout << "[CB] Brightness changed to: " << brightness << std::endl;
    });

    // Trigger the callback by changing the 'volume' value to 90
    // This will call the callback registered for 'volume'
    Configly<Config>::instance().set(&Config::volume, 90);

    // Trigger the callback by changing the 'brightness' value to 10
    // This will call the callback registered for 'brightness'
    Configly<Config>::instance().set(&Config::brightness, 10.0f);

    std::cout << "------------Set all fields------------" << std::endl;

    // Set all configuration values at once using 'setAll()'
    // This will set the 'volume' to 10 and 'brightness' to 20 in the user configuration
    Configly<Config>::instance().setAll({ 10, 20.0f });

    return 0;
}
