# Contributing — CYD CNC Controller

## Development Setup

1. Install [PlatformIO](https://platformio.org/) (CLI or IDE plugin)
2. Clone the repo
3. Copy `data/wifi.cfg.example` → `data/wifi.cfg` and set your WiFi credentials
4. `pio run` to verify it compiles

## Build Environments

| Environment | Purpose | Command |
|-------------|---------|---------|
| `esp32-2432S028R` | Production firmware for CYD board | `pio run` |
| `native` | Unit tests on PC (no hardware needed) | `pio test -e native` |

## Development Workflow

1. **Write code** following conventions below
2. **Add/update tests** for any new logic in `lib/testable/`
3. **Run tests:** `pio test -e native`
4. **Build:** `pio run` (check it compiles for ESP32)
5. **Test on hardware** if touching UI, serial, or SD code

## Code Conventions

### Naming
- `camelCase` — functions, variables, parameters
- `PascalCase` — classes, structs, enums, enum values
- `UPPER_SNAKE_CASE` — `#define` macros and constants
- `_underscorePrefix` — private/protected member variables

### Style
- 4 spaces, no tabs
- K&R braces (opening brace on same line as statement)
- `#pragma once` for all headers
- `// ====` banner comments between sections
- Max line length: 120 characters

### Module Pattern
Every module follows:
```cpp
// header.h
#pragma once
class MyModule {
public:
    void begin();       // One-time init, called from setup()
    void loop();        // Called every iteration from loop()
private:
    int _someState;
};
extern MyModule myModule;  // Global singleton

// header.cpp
#include "header.h"
MyModule myModule;         // Singleton definition
void MyModule::begin() { /* ... */ }
void MyModule::loop() { /* ... */ }
```

### Memory Rules (CRITICAL)
- **No heap allocation** — no `new`, `delete`, `malloc`, `free`
- **No STL containers** — no `std::vector`, `std::string`, `std::map`
- **No Arduino `String`** — use `char[]` with fixed sizes
- **Fixed-size buffers everywhere** — see `config.h` for max sizes
- **Stack-allocated only** — all objects are global singletons or stack variables

### Arduino/ESP32 Patterns
- Use `millis()` for timing — never `delay()` in `loop()`
- Debug logging via `DBG()` macro (only active when `DEVELOP_MODE=1`)
- WiFi credentials loaded from SD card (`/wifi.cfg`), compile-time fallback in `config.h`

## Testing

### Adding a New Test Suite

1. Create `test/test_myfeature/test_myfeature.cpp`
2. Include `<unity.h>` and `"testable_logic.h"`
3. Write test functions: `void test_something(void) { TEST_ASSERT_...; }`
4. Add `main()` function calling `UNITY_BEGIN()`, `RUN_TEST(...)`, `UNITY_END()`
5. Run: `pio test -e native -f test_myfeature`

### Test Naming Convention
```cpp
void test_<module>_<scenario>(void) {
    // Arrange
    // Act
    // Assert with TEST_ASSERT_*
}
```

## Project Documentation

| File | Purpose |
|------|---------|
| `README.md` | User-facing: setup, wiring, usage guide |
| `SPECIFICATION.md` | Detailed software specification |
| `ARCHITECTURE.md` | System architecture diagrams and data flows |
| `TEST_STRATEGY.md` | Testing approach and test case catalog |
| `CLAUDE.md` | AI assistant context (Claude, Cursor, etc.) |
| `.github/copilot-instructions.md` | GitHub Copilot context |
| `Idea.md` | Original project concept |

