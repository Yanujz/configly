# Configly - Flexible Configuration Management for C++

Configly is a flexible, callback-driven configuration management library for C++. It allows you to manage, update, and restore your configuration parameters with ease. This library is ideal for applications that need dynamic configuration management and real-time notifications on changes.

Note: Configly is not thread-safe. If you need to use it in a multi-threaded environment, make sure to synchronize access to the configuration manually.

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
#include "configly.hpp"
#include <iostream>

struct Config {
    int volume;
    float brightness;
};

Config defaultConfig = {50, 0.75f};
Config userConfig;

int main() {
    // Set default config
    Configly<Config>::instance().setDefault(defaultConfig);

    // Modify the config
    Configly<Config>::instance().set(&Config::volume, 80);

    // Register a callback for volume change
    Configly<Config>::instance().onChange(&Config::volume, [](int newVolume) {
        std::cout << "Volume changed to: " << newVolume << std::endl;
    });

    // Trigger the callback by changing the volume
    Configly<Config>::instance().set(&Config::volume, 90);

    // Output: Volume changed to: 90
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