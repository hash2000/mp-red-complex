# AGENTS.md - Development Guide for RedComplex

## Build Commands

### Requirements
- CMake 4.1+
- C++20 compatible compiler (MSVC or GCC)
- vcpkg for dependency management

### Building the Project
```bash
# Configure with CMake (using vcpkg toolchain)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Build all targets
cmake --build build --parallel

# Build specific target
cmake --build build --target ResourcesTool
cmake --build build --target Game
cmake --build build --target Launcher
```

### Running Applications
```bash
# Run ResourcesTool
./build/src/ResourcesTool/ResourcesTool

# Run Game
./build/src/Game/Game

# Run Launcher
./build/src/Launcher/Launcher
```

### Single Test Execution
**No test framework is currently configured.** To add tests:
```bash
# If GoogleTest is added later:
cmake --build build --target RunTests
./build/tests/YourTest --gtest_filter=TestName.Filter
```

---

## Code Style Guidelines

### General
- **Language**: C++20
- **Qt Framework**: Qt6 (Core, Gui, Widgets, OpenGLWidgets)
- **Dependencies**: EnTT (entity-component system), ZLIB

### File Organization
- Header files: `.h` extension
- Implementation files: `.cpp` extension
- Use `#pragma once` for header guards (not `#ifndef` guards)
- Order includes: project headers, then Qt headers, then standard library

### Naming Conventions
- **Classes**: `PascalCase` (e.g., `ApplicationController`)
- **Functions/Methods**: `camelCase` (e.g., `executeCommand`)
- **Member variables**: `_camelCase` with leading underscore (e.g., `_resources_path`)
- **Constants**: `kPascalCase` or `SCREAMING_SNAKE_CASE`
- **Files**: Match class name (e.g., `app_controller.h` / `app_controller.cpp`)

### Formatting
- **Indentation**: 2 spaces (tabs converted to spaces)
- **Line endings**: CRLF (Windows)
- **Braces**: Same-line opening brace for functions/classes
- **Trailing whitespace**: Trimmed
- **Final newline**: Required

### Imports and Forward Declarations
- Use forward declarations to avoid unnecessary includes in headers
- Group includes by category:
  1. Corresponding header (in .cpp files)
  2. Project headers (quoted)
  3. Qt headers (angle brackets)
  4. Standard library (angle brackets)

```cpp
#pragma once
#include <QObject>
#include <memory>

class MyClass;  // Forward declaration

class Foo : public QObject {
Q_OBJECT
public:
    Foo(QObject* parent = nullptr);
};
```

### Memory Management
- Use `std::unique_ptr` for exclusive ownership
- Use `std::shared_ptr` for shared ownership
- Use `std::make_unique<T>()` and `std::make_shared<T>()` for construction
- Prefer pimpl idiom (`std::unique_ptr<Private>`) to hide implementation details

```cpp
class MyClass::Private {
public:
    Private(MyClass* parent) : q(parent) {}
    MyClass* q;
    std::unique_ptr<Helper> helper;
};

MyClass::MyClass()
: QObject(parent)
, d(std::make_unique<Private>(this)) {}
```

### Types and Modern C++
- Use `std::optional<T>` instead of null pointers for nullable return values
- Use `std::span` for array views
- Use range-based for loops with `const auto&`
- Use structured bindings where appropriate

```cpp
auto result = getStream(container, path);
if (result.has_value()) {
    // use *result or result.value()
}

for (const auto& entry : entries) { }
```

### Error Handling
- Use exceptions sparingly with `std::exception` derived types
- Log errors with `qWarning()` or `qInfo()`
- Use `Q_UNUSED()` for intentionally unused parameters

```cpp
try {
    auto item = std::make_unique<DatFile>();
    item->loadFromFile(path);
}
catch (std::exception& ex) {
    qWarning() << "Failed to load:" << ex.what();
    return;
}
```

### Qt-Specific Patterns
- Use `Q_OBJECT` macro in all QObject-derived classes
- Use signals/slots for Qt observer pattern
- Use `QPointer<T>` for QObject pointers that can become invalid
- Override virtual methods with `override` specifier
- Use `QDir`, `QFileInfo` for file operations

### Logging
- `qDebug()` - Debug information
- `qInfo()` - General information
- `qWarning()` - Warning messages (non-fatal errors)
- `qCritical()` - Critical errors
- `qFatal()` - Fatal errors (aborts)

### Project Structure
```
src/
├── Base/           # Base utilities and widgets
├── BaseWidgets/    # Reusable UI components
├── ApplicationLayer/ # Application orchestration
├── Content/        # Content editors (Map, Shader, Particles)
├── DataFormat/     # Data format definitions. Legacy. 
├── DataLayer/      # Data access layer
├── DataStream/     # Stream I/O (DAT, RAW formats).
├── Engine/         # Core engine
├── Game/           # Main game application
├── Launcher/       # Launcher application. Legacy. 
├── Resources/      # Resource management
└── ResourcesTool/  # Resource viewer/editor tool. Legacy. 
```

### Common Patterns

#### Signal/Slot Connections
```cpp
connect(d->controller, &Controller::valueChanged,
        this, &MyClass::onValueChanged);

// For custom signals
signals:
    void mySignal(const QString& value);
```

#### Property System
```cpp
Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
```

---

## Development Workflow

1. **Create feature branch**: `git checkout -b feature/your-feature`
2. **Make changes** following the code style above
3. **Build and test** manually (no automated tests yet)
4. **Commit with clear messages**: `git commit -m "Description of changes"`
5. **Push and create PR** when ready
