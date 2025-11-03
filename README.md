# Configly

![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Type](https://img.shields.io/badge/Type-Header--Only-orange.svg)
![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

**Configly** is a small, header-only configuration manager for C++ aimed at embedded / real-time code where:

- config is **read very often** (fast paths, high-priority tasks, ISRs),
- config is **written rarely** (CLI, comms, low-priority tasks),
- you **cannot afford** to block readers just because someone is updating a value.

It gives you **lock-free reads** over an entire config struct, while still letting you update the whole thing or a single field and get change callbacks.

No threads spawned, no heap, no RTTI, no nonsense.

---

## Why?

The usual approach for “shared config” is:

- put everything in a struct,
- guard it with a mutex,
- hope readers don’t block.

That works on desktops, but on MCUs / RTOS you can’t have a high-priority task blocked because a low-priority task was updating a value.

Configly solves that with a **double-buffer + per-buffer sequence** design:

- writers write into the inactive buffer,
- mark it stable,
- flip an atomic index,
- readers always read a stable snapshot,
- if a reader notices a race, it just retries (no blocking).

So: **readers are never blocked by writers**.

---

## Key Features

- **Header-only** — just drop in `configly.hpp`.
- **C++17** — uses structured bindings / `std::atomic` / `std::array`.
- **Trivially copyable configs** — simple POD-style structs.
- **Lock-free read path** — readers don’t take locks.
- **Double-buffered + seqcheck** — prevents torn reads even under heavy writers.
- **Per-field callbacks** — run code when a specific member changes.
- **No heap** — suitable for embedded / safety-leaning targets.
- **Optional save/load hooks** — plug in flash/EEPROM/persistent storage.

---

## How It Works (short)

1. You define a struct, e.g.:

   ```cpp
   struct AppConfig {
       uint32_t baud;
       bool     logging;
   };
2. You tell Configly what the defaults are.
3. Readers call getAll(...) or get(&AppConfig::baud) — these are lock-free.
4. Writers call update(...) (whole struct) or set(...) (single member).
5. If a field changed and you registered a callback for it, Configly calls it.

Internally it keeps two copies of your struct and an atomic “which one is current” index. Updates go to the inactive one, then the index flips. A per-buffer sequence number makes sure a reader never consumes a half-written buffer.


## Requirements
- C++17 or newer
- `std::atomic` available
- Struct must be trivially copyable (most “config” structs are)


## Installation
### 1. Drop-in
Just copy configly.hpp somewhere in your include path:
```cpp
#include <configly/configly.hpp>
```
That’s it.
### 2. CMake (example)
```cmake
add_subdirectory(external/configly)
target_link_libraries(your_target PRIVATE configly)
```
or with `FetchContent`:
```cmake
include(FetchContent)

FetchContent_Declare(
  configly
  GIT_REPOSITORY https://github.com/Yanujz/configly.git
  GIT_TAG        main
)

FetchContent_MakeAvailable(configly)

target_link_libraries(your_target PRIVATE configly)
```

## Basic Usage
```cpp
#include <configly/configly.hpp>
#include <iostream>

struct MySettings {
    int  speed;
    bool enabled;
};

// callback for when speed changes
void onSpeedChange(const int& newSpeed, void* /*userData*/) {
    std::cout << "[callback] speed -> " << newSpeed << "\n";
}

int main() {
    // get singleton
    auto& cfg = Configly<MySettings>::instance();

    // set defaults (also initializes both buffers)
    cfg.setDefault({100, false});

    // register callback on one field
    cfg.onChange(&MySettings::speed, &onSpeedChange);

    // read a single field (lock-free)
    std::cout << "enabled = " << cfg.get(&MySettings::enabled) << "\n";

    // update one field (will trigger callback)
    std::cout << "setting speed to 9000...\n";
    cfg.set(&MySettings::speed, 9000);

    // read whole config
    MySettings snapshot{};
    cfg.getAll(snapshot);
    std::cout << "snapshot.speed = " << snapshot.speed << "\n";

    // restore defaults (also triggers callbacks for changed fields)
    cfg.restoreDefaults();
    std::cout << "after restore: speed = " << cfg.get(&MySettings::speed) << "\n";

    return 0;
}
```

## Threading / RT Notes
- Reads (getAll, get)
  - never block,
    - may retry internally if a writer reused the buffer during copy,
    - always return a consistent snapshot.
- Writes (update, set)
    - writers are serialized with an atomic_flag,
    - intended for “rare” updates from lower-priority code,
    - safe for concurrent readers.
- Callbacks
    - 1 callback per field (by design),
    - stored in a fixed array (no heap),
    - callback is called after the new config is published.

If you need “writers must never spin”, you can wrap update(...) in your own “try” function and only call it from a safe context.


## Save / Load Hooks
You can plug in your own persistence:
```cpp
bool saveToFlash(const MySettings& cfg);
bool loadFromFlash(MySettings& cfg);

auto& c = Configly<MySettings>::instance();
c.setSaveFunction(&saveToFlash);
c.setLoadFunction(&loadFromFlash);

c.save(); // reads current config and calls saveToFlash(...)
c.load(); // calls loadFromFlash(...) and updates config if ok
```
This keeps the core header free of platform-specific code.

## Building & Testing
If you cloned the repo with the tests:
```bash
git clone https://github.com/Yanujz/configly.git
cd configly
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


## Contributing
- Open an issue for bugs / questions.
- PRs welcome, but keep it small and embedded-friendly.
- No heavy dependencies, no heap, no magic codegen.