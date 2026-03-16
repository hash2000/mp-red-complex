# AGENTS.md - Development Guide for RedComplex

RedComplex — это пет-проект на C++ с использованием фреймворка Qt, представляющий собой оконное приложение с элементами игры. Основной акцент сделан на систему инвентаря и экипировки. Проект использует современные подходы C++20, включая умные указатели, сигналы/слоты Qt, и модульную архитектуру.

## Code Standards
- эталонная локаль - русская (ru)

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

### Running Tests
```bash
# Build tests target
cmake --build build --target RunTests

# Run all tests
cmake --build build --target run_tests

# Or directly via ctest
cd build
ctest --output-on-failure

# Run specific test function
./build/tests/RunTests -testfunction testLoadEntities
./build/tests/RunTests -testfunction testMoveFullStack_PreservesItemId
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
├── ResourcesTool/  # Resource viewer/editor tool. Legacy.
└── tests/          # Unit tests (Qt Test Framework)
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

#### Unit Testing (Qt Test Framework)
Tests are located in `tests/` directory and use Qt Test Framework.

**Test file structure:**
```cpp
#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"

class TestMyClass : public QObject {
    Q_OBJECT

private slots:
    void init();              // Before each test
    void cleanup();           // After each test
    void testMyFunction();    // Test function
};

void TestMyClass::testMyFunction() {
    // Arrange
    ItemsService service(dataProvider);
    
    // Act
    const auto item = service.duplicate("item-id");
    
    // Assert
    QVERIFY(item != nullptr);
    QCOMPARE(item->entityId, QString("item-id"));
}

QTEST_MAIN(TestMyClass)
#include "test_all.moc"
```

**Key test macros:**
- `QVERIFY(condition)` — Check condition is true
- `QCOMPARE(actual, expected)` — Compare values
- `QTEST_MAIN(TestClass)` — Main entry point for tests

### Architecture and Design

#### Core Application Flow
1. **Engine Layer**: The application starts in `main.cpp` where `GameApplication` inherits from `Application`. This base class handles core initialization like the Qt application, configuration loading, and resource setup.
2. **Main Window Creation**: `Application::run()` calls `createMainFrame()` (pure virtual). In `GameApplication`, this returns a `GameMainFrame`.
3. **Application Layer**: `GameApplication` is created first, then it instantiates `ApplicationController` in `GameMainFrame`'s Private class. This controller orchestrates high-level application logic.

#### Key Components

**ApplicationController** (`src/Game/app_controller.*`)
- The central coordinator. Owns and manages:
  - `CommandProcessor`: Parses and executes text commands.
  - `Services`: Holds core game services (time, world, inventory, items).
  - `Controllers`: Manages UI controllers (e.g., `WindowsController`).
- Uses the PIMPL pattern with a `Private` class for encapsulation.

**Services** (`src/Game/services.*`)
- A container for all major game systems.
- Key services include:
  - `TimeService`: A precise timer for game ticks and scheduled events.
  - `InventoriesService`: The core logic for managing inventories and equipment.
  - `ItemsService`: Manages item definitions and creation.
- Initialized with data providers (e.g., `ItemsDataProviderJsonImpl`) for loading game data from JSON files.

**Inventory and Equipment System**
- **Data Layer** (`src/DataLayer`): Provides raw data access.
  - `ItemsDataProviderJsonImpl`: Loads item definitions (`items.json`, `items_ids.json`) and icons from the assets folder.
  - `InventoryDataProviderJsonImpl` / `EquipmentDataProviderJsonImpl`: Load inventory and equipment state from JSON files in the data folder.
  - `InventoriesDataProviderJsonImpl`: Discovers which inventory and equipment files exist.
- **Application Layer** (`src/ApplicationLayer`): Implements business logic.
  - `ItemsService`: Loads all item entities and can create instances (`duplicate` method).
  - `InventoriesService`: The central hub. It uses `ItemPlacementStore` to load `ItemPlacementService` instances for specific inventories or equipment.
    - `InventoryStoreImpl` -> `InventoryService`: For regular inventories.
    - `EquipmentStoreImpl` -> `EquipmentService`: For equipment slots.
  - `ItemMimeData`: A data transfer object used for drag-and-drop operations between containers.
  - `ItemPlacementService`: An abstract interface (`pure virtual`) defining operations like `canPlaceItem`, `moveItem`, `applyDublicateFromItem`. Both `InventoryService` and `EquipmentService` implement this.
- This design allows the `InventoriesService` to handle moves between any two containers (inventory-to-inventory, inventory-to-equipment) by treating them all as `ItemPlacementService` instances.

**Item Stack Management (Пачки предметов)**
- Items can be stacked if `entity->maxStack > 1`
- When moving a **full stack** between inventories, the item ID is preserved (no duplicate created)
- When moving a **partial stack**, a new item is created with a new ID for the moved quantity
- `InventoryService::transferItem()` — Move/add item with preserved ID
- `InventoriesService::moveItem()` — Handles full/partial stack logic automatically

**Item Creation Widget**
- `ItemCreateWidget` (`src/Game/widgets/items/item_create_widget.*`) — Dialog for creating new items
- Allows selecting quantity (1 to maxStack)
- Creates items in the last active inventory window
- Uses `ItemsService::createItemByEntity()` to instantiate items from entity definitions

**Item Save System**
- `ItemsSaveManager` (`src/ApplicationLayer/items_save_manager.*`) — Saves all created items
- `ItemsDataWriterJsonImpl` — JSON writer for item persistence
- Items are saved to `data/items/{uuid}.json`
- Connected to `Services::save` signal alongside inventory/equipment saving

---

## Development Workflow

1. **Create feature branch**: `git checkout -b feature/your-feature`
2. **Make changes** following the code style above
3. **Build and test** — Use Qt Test Framework for automated testing
4. **Commit with clear messages**: `git commit -m "Description of changes"`
5. **Push and create PR** when ready
