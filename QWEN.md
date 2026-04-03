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
- **Interfaces**: `PascalCase` starts with `I` (e.g., `IInventoryRepository`)
- **Implementations: `PascalCase` ends with implementation type and `Impl` (e.g., `InventoryRepositoryJsonImpl`)
- **Functions/Methods**: `camelCase` (e.g., `executeCommand`)
- **Constants**: `kPascalCase`
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
  - `InventoriesService`: The central hub. It uses `ItemPlacementStore` to load `IItemPlacementService` instances for specific inventories or equipment.
    - `InventoryStoreImpl` -> `InventoryService`: For regular inventories.
    - `EquipmentStoreImpl` -> `EquipmentService`: For equipment slots.
  - `ItemMimeData`: A data transfer object used for drag-and-drop operations between containers.
  - `IItemPlacementService`: An abstract interface (`pure virtual`) defining operations like `canPlaceItem`, `moveItem`, `applyDublicateFromItem`. Both `InventoryService` and `EquipmentService` implement this.
- This design allows the `InventoriesService` to handle moves between any two containers (inventory-to-inventory, inventory-to-equipment) by treating them all as `IItemPlacementService` instances.

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

## Creating New MDI Windows

To add a new MDI child window to the application, follow the established pattern:

### 1. Create Data Layer (if needed)

For windows that need data persistence, create a data provider interface and implementation:

```cpp
// src/DataLayer/your_module/i_your_data_provider.h
#pragma once
#include <QString>
#include <QHash>
#include <optional>

struct YourData {
    QString id;
    QString name;
    // ... other fields
};

class IYourDataProvider {
public:
    virtual ~IYourDataProvider() = default;
    virtual std::optional<YourData> loadItem(const QString& id) const = 0;
    virtual bool saveItem(const YourData& item) = 0;
};
```

```cpp
// src/DataLayer/your_module/your_data_provider_json_impl.h
#pragma once
#include "DataLayer/your_module/i_your_data_provider.h"
#include <memory>

class Resources;

class YourDataProviderJsonImpl : public IYourDataProvider {
public:
    YourDataProviderJsonImpl(Resources* resources);
    ~YourDataProviderJsonImpl() override;

    std::optional<YourData> loadItem(const QString& id) const override;
    bool saveItem(const YourData& item) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};
```

### 2. Create Application Layer Service

Create a service that uses the data provider:

```cpp
// src/ApplicationLayer/your_module/your_service.h
#pragma once
#include "DataLayer/your_module/i_your_data_provider.h"
#include <QObject>
#include <memory>

class YourService : public QObject {
    Q_OBJECT
public:
    explicit YourService(std::unique_ptr<IYourDataProvider> dataProvider, QObject* parent = nullptr);
    ~YourService() override;

    // Business logic methods
    void doSomething();

signals:
    void dataChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};
```

### 3. Create Widget (UI Component)

Create the actual UI widget:

```cpp
// src/Game/widgets/your_module/your_widget.h
#pragma once
#include <QWidget>
#include <memory>

class YourService;
class QPushButton;
class QLineEdit;

class YourWidget : public QWidget {
    Q_OBJECT
public:
    explicit YourWidget(YourService* service, QWidget* parent = nullptr);
    ~YourWidget() override;

signals:
    void actionCompleted();

private slots:
    void onButtonClicked();

private:
    void setupLayout();

    class Private;
    std::unique_ptr<Private> d;
};
```

### 4. Create MDI Window

Create the MDI child window that wraps the widget:

```cpp
// src/Game/widgets/your_module/your_window.h
#pragma once
#include "Game/mdi_child_window.h"
#include <QObject>
#include <memory>

class YourService;
class YourWidget;

class YourWindow : public MdiChildWindow {
    Q_OBJECT

public:
    explicit YourWindow(YourService* service, const QString& id, QWidget* parent = nullptr);
    ~YourWindow() override;

    QString windowType() const override { return "your-window-type"; }
    QString windowTitle() const override { return "Window Title"; }
    QSize windowDefaultSizes() const override { return QSize(400, 300); }

    bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private slots:
    void onWidgetAction();

private:
    class Private;
    std::unique_ptr<Private> d;
};
```

```cpp
// src/Game/widgets/your_module/your_window.cpp
#include "Game/widgets/your_module/your_window.h"
#include "Game/widgets/your_module/your_widget.h"
#include "ApplicationLayer/your_module/your_service.h"
#include "Game/app_controller.h"
#include "Game/commands/command_context.h"

class YourWindow::Private {
public:
    Private(YourWindow* parent) : q(parent) {}
    YourWindow* q;
    YourService* service;
    YourWidget* widget = nullptr;
    ApplicationController* controller = nullptr;
};

YourWindow::YourWindow(YourService* service, const QString& id, QWidget* parent)
    : d(std::make_unique<Private>(this))
    , MdiChildWindow(id, parent) {
    d->service = service;
    d->widget = new YourWidget(service, this);

    connect(d->widget, &YourWidget::actionCompleted, this, &YourWindow::onWidgetAction);

    setWidget(d->widget);
    resize(windowDefaultSizes());
}

YourWindow::~YourWindow() = default;

bool YourWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
    if (commandName == "create") {
        d->controller = context->applicationController();
        return true;
    }
    return false;
}

void YourWindow::onWidgetAction() {
    // Handle widget action
}
```

### 5. Register Service in `services.cpp`

Add the service to the Services class:

```cpp
// src/Game/services.h - add forward declaration
class YourService;
class IYourDataProvider;  // Forward declaration of interface

// src/Game/services.h - add method
YourService* yourService() const;

// src/Game/services.cpp - add include
#include "ApplicationLayer/your_module/your_service.h"
#include "DataLayer/your_module/your_data_provider_json_impl.h"
#include "DataLayer/your_module/i_your_data_provider.h"

// src/Game/services.cpp - add to Private class
std::unique_ptr<YourService> yourService;

// Data providers are stored in Services (ownership)
std::unique_ptr<IYourDataProvider> yourDataProvider;

// src/Game/services.cpp - create in constructor
// First, create the data provider (Services owns it)
d->yourDataProvider = std::make_unique<YourDataProviderJsonImpl>(resources);

// Then, pass raw pointer to service (service doesn't own it)
d->yourService = std::make_unique<YourService>(d->yourDataProvider.get());

// src/Game/services.cpp - add accessor
YourService* Services::yourService() const {
    return d->yourService.get();
}
```

**Important: Service-Provider Ownership Pattern**

The project follows a specific ownership pattern for services and data providers:

1. **`Services` class owns all data providers** via `std::unique_ptr<IYourDataProvider>`
2. **Services receive non-owning raw pointers** to the interface
3. This ensures:
   - Clear ownership (Services lifetime > Service lifetime)
   - No double-deletion issues
   - Easy testing (can inject mock providers)
   - Consistent architecture across all services

```cpp
// ❌ WRONG: Service takes ownership
d->yourService = std::make_unique<YourService>(
    std::make_unique<YourDataProviderJsonImpl>(resources));

// ✅ CORRECT: Services owns provider, service gets pointer
d->yourDataProvider = std::make_unique<YourDataProviderJsonImpl>(resources);
d->yourService = std::make_unique<YourService>(d->yourDataProvider.get());
```

**Service Header Pattern:**
```cpp
// src/ApplicationLayer/your_module/your_service.h
#pragma once
#include "DataLayer/your_module/i_your_data_provider.h"
#include <QObject>
#include <memory>

class YourService : public QObject {
    Q_OBJECT
public:
    // Service receives raw pointer (non-owning)
    explicit YourService(IYourDataProvider* dataProvider, QObject* parent = nullptr);
    ~YourService() override;

    // ...
};
```

**Service Implementation Pattern:**
```cpp
// src/ApplicationLayer/your_module/your_service.cpp
class YourService::Private {
public:
    Private(YourService* parent) : q(parent) {}
    YourService* q;
    IYourDataProvider* dataProvider = nullptr;  // Raw pointer, not owned
};

YourService::YourService(IYourDataProvider* dataProvider, QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>(this)) {
    d->dataProvider = dataProvider;  // Just store the pointer
}
```

---

### Textures and Images Service

For loading and caching textures/icons, use the `TexturesService`:

**Data Layer** (`src/DataLayer/textures/`):
- `ITexturesDataProvider` — Interface for loading textures
- `TexturesDataProviderJsonImpl` — Implementation that loads from `assets/icons/`

**Application Layer** (`src/ApplicationLayer/textures/`):
- `TexturesService` — Service with caching (QHash for icons and textures)

**Usage:**
```cpp
// Get icon (automatically cached)
auto* texturesService = services->texturesService();
QPixmap icon = texturesService->getIcon("login.png");  // Loads from assets/icons/login.png

// Get texture by path
QPixmap texture = texturesService->getTexture("textures/ground.png");

// Preload icon to cache
texturesService->preloadIcon("sword.png");

// Clear all caches
texturesService->clearCache();
```

**Key Features:**
- Automatic caching prevents repeated disk reads
- Returns stub icon if file not found
- Single point of configuration for all image loading

### 6. Register Window in `windows_builder.cpp`

Add the window factory:

```cpp
#include "Game/widgets/your_module/your_window.h"

// In constructor:
d->factory.emplace("your-window-type", [](Services* services, const QString& id) {
    return new YourWindow(
        services->yourService(),
        id);
});
```

### 7. Create Window via Command

```cpp
// Via code:
controller->executeCommandByName("window-create", QStringList{ "your-window-type", "unique-id" });

// Via console:
window-create your-window-type unique-id
```

### Example: Login Window

The Login window (`src/Game/widgets/user/`) demonstrates the complete pattern:

1. **Data Layer**: `IUsersDataProvider`, `UsersDataProviderJsonImpl`
2. **Service**: `UsersService` with `login()`, `logout()`, `registerUser()` methods
3. **Widget**: `LoginWidget` with login/password fields
4. **Window**: `LoginWindow` wrapping the widget
5. **Registration**: Added to `services.cpp` and `windows_builder.cpp`

---

## Widget Styling

Проект использует единую цветовую палитру (Tailwind Slate) для всех виджетов.

### Color Palette

| Элемент | Цвет |
|---------|------|
| Основной фон | `#1a202c` |
| Фон панелей/рамок | `#2d3748` |
| Рамки | `#4a5568` |
| Рамки (hover) | `#718096` |
| Текст основной | `#e2e8f0` |
| Текст вторичный | `#a0aec0` |
| Текст подсказок | `#718096` |

### EquipmentWidget Pattern

```cpp
setObjectName("WidgetName");
setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
setLineWidth(1);

setStyleSheet(R"(
    #WidgetName {
        background-color: #1a202c;
        border: 1px solid #4a5568;
        border-radius: 8px;
        padding: 12px;
    }
)");
```

### Standard Widget Styles

**Заголовок/панель:**
```cpp
"background-color: #2d3748; "
"border: 1px solid #4a5568; "
"border-radius: 4px; "
"padding: 6px;"
```

**Иконки/аватары:**
```cpp
"background-color: #1a202c; "
"border: 1px solid #4a5568; "
"border-radius: 4px;"
```

**Метки с информацией (ID, уровень):**
```cpp
"background-color: #1a202c; "
"color: #a0aec0; "
"padding: 1px 6px; "
"border: 1px solid #4a5568; "
"border-radius: 3px; "
"font-size: 10px; "
"font-family: monospace;"
```

**Кнопки-иконки (UTF-8 символы):**
```cpp
"QPushButton { "
"  background-color: #2d3748; "
"  color: #e2e8f0; "
"  border: 1px solid #4a5568; "
"  border-radius: 3px; "
"  font-size: 12px; "
"}"
"QPushButton:hover { "
"  background-color: #4a5568; "
"  border: 1px solid #718096; "
"}"
"QPushButton:pressed { "
"  background-color: #1a202c; "
"}"
```

**Скроллбар:**
```cpp
"QScrollArea { "
"  background-color: #1a202c; "
"  border: none; "
"  border-left: 2px solid #4a5568; "
"  padding-left: 4px; "
"}"
"QScrollBar:vertical { "
"  background-color: #2d3748; "
"  width: 8px; "
"  border-radius: 4px; "
"}"
"QScrollBar::handle:vertical { "
"  background-color: #4a5568; "
"  border-radius: 4px; "
"  min-height: 20px; "
"}"
"QScrollBar::handle:vertical:hover { "
"  background-color: #718096; "
"}"
"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
"  height: 0px; "
"}"
```

**Разделители:**
```cpp
line->setFrameShape(QFrame::HLine);
line->setStyleSheet("background-color: #2d3748;");
```

### Layout Guidelines

- **Отступы виджетов**: `8px` по краям, `8px` между элементами
- **Отступы записей списка**: `6px` горизонтально, `4px` вертикально
- **Список персонажей**: без рамки, линия слева `border-left: 2px solid #4a5568`
- **Кнопки-иконки**: `24x24`, UTF-8 символы, подсказки через `setToolTip()`
- **Выравнивание**: `Qt::AlignTop` для вертикального расположения элементов

### Example: User Profile Widget

```
UserWidget (QFrame)
├── Header (QFrame, #2d3748)
│   ├── Icon (40x40, #1a202c)
│   └── Info
│       ├── UserId (monospace, #a0aec0)
│       └── DisplayName (bold, #e2e8f0)
└── ScrollArea (#1a202c, border-left only)
    └── CharactersContainer
        ├── CharacterEntryWidget
        │   ├── Icon (48x48)
        │   ├── Info (level + name)
        │   └── Buttons (⚔, 🎒, 24x24, vertical)
        ├── Separator (#2d3748)
        └── CharacterEntryWidget
```

---

## Development Workflow

1. **Create feature branch**: `git checkout -b feature/your-feature`
2. **Make changes** following the code style above
3. **Build and test** — Use Qt Test Framework for automated testing
4. **Commit with clear messages**: `git commit -m "Description of changes"`
5. **Push and create PR** when ready
