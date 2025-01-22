# Configly - Flexible Configuration Management for C++

Configly is a flexible, callback-driven configuration management library for C++. It allows you to manage, update, and restore configuration parameters with ease. This library is ideal for applications that require dynamic configuration management and real-time notifications for configuration changes.

Configly provides a centralized approach to configuration, enabling the sharing of configuration state across the entire application. All components within the program can access and modify the configuration without needing to duplicate or manually pass data. Any changes made to the configuration are immediately visible throughout the program, making it easier to handle dynamic parameters.


## Features
- Dynamic configuration management with flexible data types.
- Real-time notifications via callbacks when parameters change.
- Easy-to-use API for setting, getting, and restoring configuration parameters.
- Ideal for embedded systems and applications where settings need to be managed efficiently.

## Installation

### Using CMake
To use **Configly** in your project, you can simply include it as a header-only library.
d
1. Clone this repository:
```bash
   git clone https://github.com/Yanujz/configly.git
```
2. Include the header file in your project:
```
#include "path/to/configly.hpp"
```

3.
```
add_subdirectory(path/to/configly)
target_link_libraries(your-project-name configly)
```
## Example Usage
Here is a basic example of how to use Configly to manage configuration parameters:

```cpp
#include <iostream>
#include "configly.hpp"

// Define the configuration structure that holds settings
struct Config
{
    int   volume;  
    float brightness;
};

// Default configuration values
Config defaultConfig = { 50, 0.75f };  // Default volume is 50, default brightness is 0.75
Config userConfig;  // User-defined configuration (initially empty)

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

    // Modify the 'volume' configuration parameter using the 'set()' function
    // This will set the volume to 80 in the user configuration
    Configly<Config>::instance().set(&Config::volume, 80);

    // Register callback functions to handle changes to the 'volume' and 'brightness' parameters.
    // These functions will be called when the respective parameters are changed.
    Configly<Config>::instance().onChange(&Config::volume, [](int &newVolume) -> void {
        // Callback for volume changes
        std::cout << "Volume changed to: " << newVolume << std::endl;
    });

    Configly<Config>::instance().onChange(&Config::brightness, [](float &brightness) -> void {
        // Callback for brightness changes
        std::cout << "Brightness changed to: " << brightness << std::endl;
    });

    // Trigger the callback by changing the 'volume' value to 90
    // This will call the callback registered for 'volume'
    Configly<Config>::instance().set(&Config::volume, 90);

    // Trigger the callback by changing the 'brightness' value to 10
    // This will call the callback registered for 'brightness'
    Configly<Config>::instance().set(&Config::brightness, 10);

    // Set all configuration values at once using 'setAll()'
    // This will set the 'volume' to 10 and 'brightness' to 20 in the user configuration
    Configly<Config>::instance().setAll({ 10, 20 });

    return 0;
}

```
In this example, the Config struct holds configuration parameters like volume and brightness. You can set defaults, modify values, and register callbacks to listen for changes in real-time.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


## Contributing
Contributions are welcome! If you find a bug or have a feature request, please open an issue or submit a pull request.

### Steps to contribute

1. Fork this repository.
2. Create a new branch (**git checkout -b feature-name**).
3. Commit your changes (**git commit -am 'Add feature'**).
4. Push to the branch (**git push origin feature-name**).
5. Create a new pull request.