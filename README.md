# Configly - 

![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Type](https://img.shields.io/badge/Type-Header--Only-orange.svg)
![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

**Configly** is a professional, header-only, type-safe, and thread-safe configuration manager for C++, designed specifically for embedded and real-time systems.

It is a scalpel, not a hammer. It is the ideal solution when you need to access shared configuration data from multiple contexts (e.g., RTOS tasks, ISRs) without introducing blocking, latency, or race conditions.

## Core Philosophy: The Right Tool for Concurrency

In many systems, configuration data is **written rarely** but **read frequently** from multiple high-priority tasks. Using a traditional mutex (a hammer) to protect this data is inefficient, as it can block a critical reader task even when no writing is occurring, leading to jitter and priority inversion.

Configly uses a **seqlock** mechanism (a scalpel). This allows for virtually unlimited readers to access the configuration data **without ever being blocked**. Writers are also highly efficient, ensuring minimal impact on the system. This makes Configly perfect for high-performance, real-time applications.

## Key Features

* **Zero Heap Allocation**: Predictable, deterministic behavior suitable for high-reliability systems where dynamic memory is forbidden.
* **Thread-Safe by Design**: Guarantees safe concurrent access with a lock-free read path.
* **Type-Safe API**: Catches type-related errors at compile-time, not at runtime.
* **Header-Only**: Just include `configly.hpp` in your project to get started.
* **Modern C++**: Built with C++17 features for a clean, safe, and expressive API.
* **Tested**: Includes a comprehensive test suite to verify correctness and concurrency safety.

## Installation

Configly is a header-only library, making integration simple.

### 1. Simple Integration (Copy-Paste)

1.  Copy the `include/configly` directory into your project's include path.
2.  Include the header in your source files:
    ```cpp
    #include <configly/configly.hpp>
    ```

### 2. CMake Integration (Recommended)

You can integrate Configly using CMake's `add_subdirectory` or `FetchContent`.

**Using `add_subdirectory`:**

1.  Clone this repository into your project (e.g., in a `lib/` folder).
2.  In your main `CMakeLists.txt`:
    ```cmake
    # Add the configly directory to your build
    add_subdirectory(lib/configly)

    # Link configly to your target
    target_link_libraries(your_project_name PRIVATE configly)
    ```

**Using `FetchContent`:**

```cmake
include(FetchContent)

FetchContent_Declare(
  configly
  GIT_REPOSITORY [https://github.com/Yanujz/configly.git](https://github.com/Yanujz/configly.git)
  GIT_TAG        main # Or a specific release tag
)

FetchContent_MakeAvailable(configly)

target_link_libraries(your_project_name PRIVATE configly)
```

## Quick Example
```cpp
#include <configly/configly.hpp>
#include <iostream>

// 1. Define your configuration struct
// It must be trivially copyable
struct MySettings {
    int   speed;
    bool  enabled;
};

// 2. Define a type-safe callback function
void onSpeedChange(const int& newSpeed, void* userData) {
    std::cout << "Callback: Speed has been changed to " << newSpeed << std::endl;
}

int main() {
    std::cout << "--- Configly Quick Example ---" << std::endl;

    // 3. Get the singleton instance
    auto& settings = Configly<MySettings>::instance();

    // 4. Set the default configuration
    settings.setDefault({ 100, false });

    // 5. Register a callback to listen for changes
    settings.onChange(&MySettings::speed, &onSpeedChange);

    // 6. Set a new value (this will trigger the callback)
    std.cout << "\nUpdating speed to 9000..." << std::endl;
    settings.set(&MySettings::speed, 9000);

    // 7. Get a value in a thread-safe manner
    bool isEnabled = settings.get(&MySettings::enabled);
    std.cout << "Current 'enabled' state: " << (isEnabled ? "true" : "false") << std::endl;

    // 8. Restore all settings to their default values
    std::cout << "\nRestoring defaults..." << std::endl;
    settings.restoreDefaults(); // This will trigger the speed callback again

    std::cout << "Speed after restore: " << settings.get(&MySettings::speed) << std::endl;

    return 0;
}
```


## Building & Testing
```bash
# 1. Clone the repository
git clone https://github.com/Yanujz/configly.git
cd configly

# 2. Configure the project with CMake
mkdir build
cd build
cmake ..

# 3. Compile the library and tests
make

# 4. Run the tests
ctest --verbose
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


## Contributing
Contributions are welcome! If you find a bug or have a feature request, please open an issue or submit a pull request.

### Steps to contribute

1. Fork this repository.
2. Create a new branch (**git checkout -b feature/your-feature-name**).
3. Commit your changes (**git commit -am 'Add some feature'**).
4. Push to the branch (**git push origin feature-name**).
5. Create a new pull request.