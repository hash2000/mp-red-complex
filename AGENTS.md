# Контекст проекта RedComplex для SourceCraft Code Assistant

RedComplex — это multiapp на C++20 с Qt6, модульной архитектурой и акцентом на extensibility. Текущая версия — modular multiapp, not a game, with pluggable modules, data libraries, and a shared engine layer.

## Языковые стандарты
- Эталонная локаль - русская (ru)
- Все комментарии и документация предпочтительно на русском языке

## Команды сборки

### Требования
- CMake 4.1+
- Компилятор с поддержкой C++20 (MSVC или GCC)
- vcpkg для управления зависимостями

### Сборка проекта
```bash
# Конфигурация с CMake (используя toolchain vcpkg)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Сборка всех целей
cmake --build build --parallel

# Сборка конкретной цели
cmake --build build --target Launcher
```

### Запуск приложений
```bash
# Запуск Launcher
./build/{Configuration}/Launcher

# Пример:
# Configuration = x64-Clang-Debug
# Запуск:
# ./build/x64-Clang-Debug/Launcher
```

Для запуска в отладочном режиме требуется указать переменную среды `QT_PLUGIN_PATH`, если для x64 windows, и проект собран через Clang, то
`QT_PLUGIN_PATH`='build\x64-Clang-Debug\vcpkg_installed\x64-windows\debug\Qt6\plugins'


### Запуск тестов
```bash
# Сборка тестов
cmake --build build --target RunTests

# Запуск всех тестов
cmake --build build --target run_tests

# Или напрямую через ctest
cd build
ctest --output-on-failure

# Запуск конкретной тестовой функции
./build/tests/RunTests -testfunction testLoadEntities
./build/tests/RunTests -testfunction testMoveFullStack_PreservesItemId
```

---

## Стандарты кода

### Общее
- **Язык**: C++20
- **Фреймворк Qt**: Qt6 (Core, Gui, Widgets, OpenGLWidgets)
- **Зависимости**: EnTT (entity-component system), ZLIB

### Организация файлов
- Заголовочные файлы: расширение `.h`
- Файлы реализации: расширение `.cpp`
- Использовать `#pragma once` для защиты заголовков (не `#ifndef`)
- Порядок включения: заголовки проекта, затем заголовки Qt, затем стандартная библиотека
- Стараться прописывать реализацию максимально в .cpp, .h должны содержать только объявления, где это возможно, кроме случаев, когда объявление и реализация приватные на уровне .cpp (пример: class MyClass::Private, всегда пишется в .cpp)

### Соглашения об именовании
- **Классы**: `PascalCase` (например, `ApplicationController`)
- **Интерфейсы**: `PascalCase` с префиксом `I` (например, `IInventoryRepository`)
- **Реализации**: `PascalCase` с суффиксом `Impl` (например, `InventoryRepositoryJsonImpl`)
- **Функции/Методы**: `camelCase` (например, `executeCommand`)
- **Константы**: `kPascalCase`
- **Файлы**: соответствуют имени класса (например, `app_controller.h` / `app_controller.cpp`)

### Форматирование
- **Отступы**: 2 пробела (табы преобразуются в пробелы)
- **Окончания строк**: CRLF (Windows)
- **Фигурные скобки**: открывающая скобка на той же строке для функций/классов
- **Лишние пробелы в конце строк**: удаляются
- **Финальный перевод строки**: обязателен
- **Операторы инкремента/декремента**: использовать постфиксную форму (`x++`, `x--`) вместо префиксной (`++x`, `--x`)

```cpp
for (int z = 0; z < 16; z++) // Да
for (int z = 0; z < 16; ++z) // Нет
```

### Импорты и предварительные объявления
- Использовать предварительные объявления, чтобы избежать лишних включений в заголовках
- Группировать включения по категориям:
  1. Соответствующий заголовок (в .cpp файлах)
  2. Заголовки проекта (в кавычках)
  3. Заголовки Qt (в угловых скобках)
  4. Стандартная библиотека (в угловых скобках)

```cpp
#pragma once
#include <QObject>
#include <memory>

class MyClass;  // Предварительное объявление

class MyClass : public QObject {
Q_OBJECT
public:
    MyClass(QObject* parent = nullptr);
		~MyClass() override;
};
```

### Управление памятью
- Использовать `std::unique_ptr` для эксклюзивного владения
- Использовать `std::shared_ptr` для разделяемого владения
- Использовать `std::make_unique<T>()` и `std::make_shared<T>()` для создания
- Предпочитать идиому pimpl (`std::unique_ptr<Private>`) для сокрытия деталей реализации

```cpp
class MyClass::Private {
public:
    Private(MyClass* parent) : q(parent) {}
    MyClass* q;
    std::unique_ptr<Helper> helper;
};

MyClass::MyClass()
: QObject(parent)
, d(std::make_unique<Private>(this)) {
}

MyClass::~MyClass() = default; // or implementation
```

### Типы и современный C++
- Использовать `std::optional<T>` вместо нулевых указателей для nullable возвращаемых значений
- Использовать `std::span` для представления массивов
- Использовать range-based for циклы с `const auto&`
- Использовать structured bindings где уместно

```cpp
auto result = getStream(container, path);
if (result.has_value()) {
    // использовать *result или result.value()
}

for (const auto& entry : entries) { }
```

### Обработка ошибок
- Использовать исключения экономно с типами, производными от `std::exception`
- Логировать ошибки с помощью `qWarning()` или `qInfo()`
- Использовать `Q_UNUSED()` для намеренно неиспользуемых параметров

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

### Qt-специфичные паттерны
- Использовать макрос `Q_OBJECT` во всех классах, производных от QObject
- Использовать сигналы/слоты для паттерна наблюдателя Qt
- Использовать `QPointer<T>` для указателей на QObject, которые могут стать невалидными
- Переопределять виртуальные методы с указателем `override`
- Использовать `QDir`, `QFileInfo` для файловых операций

### Логирование
- `qDebug()` - отладочная информация
- `qInfo()` - общая информация
- `qWarning()` - предупреждения (некритические ошибки)
- `qCritical()` - критические ошибки
- `qFatal()` - фатальные ошибки (прерывание)

### Структура проекта
* src/Content/{ModuleName} - модули системы, сейчас подключаются статически, в дальнейшем будет реализовываться система динамической подгрузки
* src/Libs/Base - базовые, независимые классы и методы
* src/Libs/BaseWidgets - базовые виджеты системы
* src/Libs/DataFormat - legacy блок, сейчас тут модели и методы десериализации .dat файлов от игры fallout 2, в дальнейшем будут вынесены в отдельный модуль
* src/Libs/DataStream - библиотека с базовыми методами для работы с бинарными данными, была сделана в связке с DataFormat, сейчас от неё ухожу, будет вынесена в отдельный модуль вместе с DataFormat
* src/Libs/Engine - ядро системы, тут система работы со скриптами, командами, конфигурациями, виджеты главного окна, QApplication
* src/Libs/Graphics - подразумевается, как библиотека с виджетами и классами для работы с графикой
* src/Libs/Resources - библиотека по управлению и хранению глобальных ресурсов системы - переменные, директории, встроенная база данных
* src/Configurations - базовая конфигурация системы, будет устанавливаться вместе с готовым приложением как эталонная. тут скрипты, и yaml файлы с описанием частей системы. подразумевается , что будет модифицироваться пользовательская копия конфигурации
* src/Legacy - старые исходники, в них есть решения , которые будут возвращены в основной проект, после того как будет обновлена система загрузки приложения. не включена в цепочку компиляции.
* src/externals - внешние зависимости, которых нет в vcpkg. при работе с зависимостями - vcpkg предпочтительнее, всегда стараться брать из него.

### Распространённые паттерны

#### Соединения сигналов/слотов
```cpp
connect(d->controller, &Controller::valueChanged,
        this, &MyClass::onValueChanged);

// Для пользовательских сигналов
signals:
    void mySignal(const QString& value);
```

#### Система свойств
```cpp
Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
```

#### Модульное тестирование (Qt Test Framework)
Тесты расположены в директории `tests/` и используют Qt Test Framework.

**Структура тестового файла:**
```cpp
#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"

class TestMyClass : public QObject {
    Q_OBJECT

private slots:
    void init();              // Перед каждым тестом
    void cleanup();           // После каждого теста
    void testMyFunction();    // Тестовая функция
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

**Ключевые макросы тестов:**
- `QVERIFY(condition)` — Проверить, что условие истинно
- `QCOMPARE(actual, expected)` — Сравнить значения
- `QTEST_MAIN(TestClass)` — Точка входа для тестов

### Архитектура и дизайн

#### Слои и зависимости

RedComplex — multiapp: общая оболочка и движок отделены от прикладных модулей, зависимости направлены только вниз.

```
Launcher              точка входа и композиция: main.cpp, ApplicationController, ServicesFactory, окна MDI
 └─ Content/{Module}  модули: команды, сервисы, провайдеры данных, модели, виджеты, миграции
     └─ Libs/Engine   Application, MainWindow, команды и контексты, парсер DSL, темы, реестры
         ├─ Libs/Resources   Variables/Directories, SQLite (SQLCipher), миграции, WAL
         ├─ Libs/Graphics    OpenGL-виджет, камера, шейдеры, текстуры, тайлы
         ├─ Libs/BaseWidgets MdiArea, lazy tree, панель свойств, layout-ы
         └─ Libs/Base        утилиты, исключения, crypto, container_view
```

- `src/Launcher` → цель `Launcher` (`qt_add_executable`): `LauncherApplication`, `ApplicationController`, `Controllers`, `Services` (фабрика сервисов), `WindowsBuilder`, команда `windows` и конкретные MDI-окна.
- `src/Libs/Engine` → статическая `RedComplexEngine`: цикл запуска, `MainWindow`, система команд и контекстов, парсер DSL, менеджер тем, `ServicesRegistry` и `WidgetRegistry`.
- `src/Content/*` → статические `RedComplex<Module>`: ConsoleModule, DatabaseModule, UsersModule, FetchApiModule, MessagerModule, CodeEditorWidget.
- `src/Libs/*` → инфраструктура, не знающая о прикладных модулях (Base, BaseWidgets, Resources, Graphics, DataStream, DataFormat).
- `src/externals` — внешние зависимости, которых нет в vcpkg (сейчас `FastNoiseLite`, header-only).
- Состав сборки задают `src/Libs/CMakeLists.txt` и `src/Content/CMakeLists.txt`, линковка — `src/Launcher/CMakeLists.txt`; пакеты ищутся в `cmake/modules.cmake` (Qt6 Core/Gui/Widgets/OpenGL/OpenGLWidgets/Network/Xml/Svg/Concurrent, ZLIB, EnTT, sqlcipher, OpenSSL, libsodium, yaml-cpp).
- Исходники собираются через `file(GLOB_RECURSE SRCS .../*.cpp)`; заголовки доступны через `include_directories(src)` из корневого `CMakeLists.txt`. Новый .cpp в существующем каталоге подхватывается автоматически, новый каталог-модуль нужно добавить в `add_subdirectory`.

#### Основной поток запуска

1. `main()` (`src/Launcher/main.cpp`) создаёт `LauncherApplication` — наследника `Application` — и вызывает `run(argc, argv)`.
2. `Application::run()` (`src/Libs/Engine/application.cpp`) оборачивает запуск в try/catch: любое `std::exception` приводит к `qFatal()`.
3. `Application::Private::tryRun()` выполняет инициализацию:
   - создаёт `Resources`, настраивает `QSurfaceFormat` (OpenGL 3.3 Core, depth 24, MSAA 4, double buffering), атрибуты Qt (HighDPI, `AA_ShareOpenGLContexts`, `QSettings::IniFormat`) и создаёт `QApplication`;
   - `setupResources()`: `Resources::load()` → `registerTypes()` (`AssetsViewItemType`, `ProgramOpCode`) → `BootstrapManager::reload()` (тема + сохранение конфигурации `bootstrap.yaml`);
   - вызывает `buildCommandController(resources)` — точку расширения приложения; `LauncherApplication` создаёт здесь `ApplicationController` и вызывает `init()`;
   - создаёт `BootstrapContextBuilder(commandController)` и вызывает `reload(bootstrapConfig)` — появляются контексты `.store.<name>` с переменными из `bootstrap.yaml`;
   - создаёт `ClassManager(resources, commandController)` и вызывает `createViewport(config.application)` — оболочка строится по `applications/{name}/main.yaml`; если описание недоступно, берётся `MainWindow` со встроенными панелями (fallback);
   - показывает вьюпорт и уходит в `QApplication::exec()`.
4. `ApplicationController::init()` (это `CommandController::init()` + `initContext()`) создаёт `CommandProcessor`, `Controllers` (`WindowsController`, `ActionPanelController`) и `Services` (фабрику), регистрирует команды `help`, `echo`, `windows`, `users`, `fetch-api`, а `WidgetsBuilder` заполняет `WidgetRegistry` нативными классами виджетов.
5. `BootstrapManager::reload()` читает `bootstrap.yaml` из `path.profile`, применяет тему (`ThemeManager` + `PaletteParser`) и отдаёт конфигурацию через `config()`; декларативный UI грузит уже `ClassManager`.
6. Виртуального `createMainFrame()` в текущем коде нет. `LauncherMainFrame` (`src/Launcher/main_frame.*`) — предыдущая оболочка на `MdiArea`, в текущем запуске не инстанцируется, но компилируется.

#### Команды, контексты и инструкции

- `CommandController` (`Libs/Engine/scripting/command_controller.*`) — владелец `globalContext()`, `WidgetRegistry` и `Resources`. `execute(text)` и `executeCommand(name, args)` создают локальный дочерний контекст и передают его в `CommandProcessor`. Вывод — только через `print*`, которые эмитят `outputRequested` через `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`, поэтому печатать можно из любого потока.
- `CommandContext` (`scripting/command_context.*`) — узел дерева контекстов: глобальный имеет пустое имя и namespace `.`; `createScopedContext(name)` создаёт дочерний и регистрирует его в глобальном контексте по namespace (`.`-цепочка, например `.test1.test2`); `context(ns)` ищет по namespace, `nameSpaces()` перечисляет все. Владение — `std::shared_ptr`, ссылки на детей — слабые (поэтому владелец контекста обязан держать `shared_ptr`, иначе узел исчезает).
- Переменные контекста: имя — произвольная строка, точки допустимы (`wal.checkInterval`). API: `setVariable`, `variable`, `hasVariable`, `variables`, `variableNames`; значения — `QVariant`.
- `CommandContext::VariablesMode` задаёт отношение переменных к базовому контексту и может меняться в любой момент: `Lazy` — читает базовое значение, при записи делает свою копию; `Mutable` — читает и пишет общую переменную базового контекста; `ReadOnly` — запись запрещена (`setVariable` возвращает `false`); `Atomic` — при переключении получает копию видимых переменных и дальше живёт отдельно. Собственные переменные контекста изолированы в любом режиме.
- `BootstrapContextBuilder` (`scripting/bootstrap/bootstrap_context_builder.*`) — создаёт по `bootstrap.yaml` контексты `.store.<name>` (`fetch_api`, `messages`, `accounts`) с переменными `wal.checkInterval` и `wal.maxSizeMb`; режим хранилищ — `ReadOnly`, `reload()` идемпотентен и обновляет значения.
- `CommandProcessor` (`scripting/command_processor.*`) — реестр `std::map<QString, std::unique_ptr<ICommand>>` (имена в нижнем регистре), разбор строки, проверка `minArgs`/`maxArgs`, `try/catch` вокруг `ICommand::execute`, замер времени и сигналы `commandExecuted`/`commandError`.
- `Instruction` (`scripting/instruction.h`) — имя команды, `QMap<QString, QVariant>` параметров и карта mime-типов; хелперы `text()`, `json()`, `xml()`, `ini()`, `hasParameter()`.
- `ICommand` (`scripting/i_command.h`) — `name()`, `description()`, `help()`, `minArgs()`, `maxArgs()`, `execute(instruction, context)`. Команда не знает, откуда вызвана: доступ к приложению — через контекст.
- `ScriptProgram` (`scripting/program.h`) — контейнер инструкций, заготовка исполняемого скрипта.
- Синтаксис строки: `command param:value param2:"текст с пробелами"`; параметры разделяются пробелами, `\` экранирует символ; блочные значения — `param:<json>{...}</json>`, `param:<xml>...</xml>`, `param:<ini>k=v</ini>`, `param:<text>...</text>`. Эквивалент из кода — `executeCommand(name, QMap<QString, QString>)`.

#### Парсер DSL

- `SourceCursor` (`scripting/parser/source_cursor.*`) — курсор по исходнику: позиция, `SourceLocation`, пропуск пробелов и комментариев (`//`, `/* */`).
- `IParser` (`parser/i_parser.h`) — `parse(SourceCursor&, std::optional<ParseError>&)` возвращает `IAstPtr` либо `nullptr` (не подошло или ошибка), `tokens()` отдаёт статические токены.
- `IAst`/`AstBase` (`parser/i_ast.h`, `parser/base/ast_base.*`) — узел AST: параметры, типизированные дети (`setChild`/`children`) и локация.
- Реализации в `parser/base`: `ParserValue`, `ParserNumber`, `ParserVariable`, `ParserAttributesCollection`, `ParserAttribute`, `ParserInclude`, `ParserBindings`, `ParserIdentifiersSpace`, `ParserPass` — собираются композицией по принципу «первый подходящий парсер».
- Поддерживаемый синтаксис описан в `parser/README.md`: типы `var`/`string`/`bool`/`vec`/`number`, атрибуты в `[...]`, биндинги `{{.var}}`, директива `#include "..."`, namespace-пути через `.`.
- Тесты парсера: `tests/engine/test_command_parsers.cpp`, `test_command_parsers_palette.cpp`, `test_command_context.cpp`.

#### Ресурсы, профиль и базы данных

- `Resources` (`Libs/Resources/resources.*`) — глобальное состояние: `Variables` (`VariablesContext`, `QVariantMap`), `Directories` (`DirectoriesContext`, ключ → `QDir`) и список загруженных `DataStreamContainer`.
- `Resources::load()` задаёт базовые пути: `path.bootstrap` (текущий каталог), `path.profile` (`QStandardPaths::AppConfigLocation` — рабочая копия конфигурации), `path.tools`, `path.databases`, `path.databases.settings.file` (`data/databases.json`), `path.databases.accounts.file`, `path.databases.fetch_api.file`, `path.users`, `path.auth.words`. `loadProfile(userHash)` пересоздаёт пути профиля `path.users/{hash}`.
- Эталонная копия конфигурации — `src/Configurations` (`bootstrap.yaml`, `applications/`, `assets/themes/`, `dataflow/`); приложение читает копию в профиле.
- Доступ к БД — только через `DatabasesService::connection(alias)` (`Content/DatabaseModule`): таблица псевдонимов (`accounts`, `game`, `fetch_api`, `messanger`) с флагом шифрования, ленивое открытие, `SQLiteConnection` (SQLCipher, ключ из `SecureBuffer`), `SQLiteWalManager` (авто-checkpoint по интервалу и размеру), `MigrationManager` (версии, up/down, откат). При `QApplication::aboutToQuit` — финальный checkpoint, `PRAGMA optimize`, закрытие.
- Схемы БД описываются кодом: `Content/DatabaseModule/migrations/{accounts,fetch_api,game}_migrations.cpp`, регистрация — в конструкторе `DatabasesService`.
- Низкоуровневые обёртки — `Libs/Resources/db/sqlite`: `SQLiteConnection`, `SQLiteReader`, `MigrationManager`/`Migration`, `SQLiteWalManager`.

#### Модули и сервисный реестр

- Типовая структура модуля `src/Content/{Module}`: `commands/` (команды консоли), `services/` (логика, `QObject`), `data_providers/` (интерфейсы `I*` и реализации `*Impl`/`*Db`), `models/`, `widgets/`, при необходимости `migrations/`.
- Сервисы не создаются напрямую: единственная точка сборки — `ServicesFactory` в `src/Launcher/services.cpp` (наследует `ServicesRegistry` и `IServicesFactory`). Регистрация — `registerFactory<T>(lambda)`, зависимости — `LazyPtr<T>` (`std::call_once`) и `get<T>()`. Доступ из команд и окон — `commandController->services()->get<T>()`.
- `ServicesRegistry` (`Libs/Engine/services/services_registry.h`) — реестр «тип → экземпляр»: `get<T>()` создаёт сервис по фабрике при первом обращении и возвращает `nullptr`, если тип не зарегистрирован.
- `IServicesFactory`/`LazyPtr` (`services/i_services_factory.h`) — ленивая инициализация зависимостей без ручного порядка объявления.
- Зарегистрированы, в частности: `DatabasesService`, `UsersService`, `UsersRegistryService`, `FetchApiService`, `FetchStoreDbService`, `HighlightingPluginManager`, `FormatterPluginManager`; провайдеры `DatabaseSettingsDataProviderJsonImpl`, `UsersRegistryDataProviderDb`.

#### UI-оболочка (Libs/Engine/ui)

- `MainWindow` (`ui/main_window.*`) — IDE-подобная композиция: слева `DocumentTabsPanel`, в центре `DocumentArea` с `DocumentWidget`/`TextDocument`, справа `SideHostPanel` + `ActivityBar`, снизу `BottomPanel`. Меню «Файл» и «Вид», статус-бар, `log()`.
- Регистрация панелей: `registerSidePanel(id, glyph, title, widget)` и `registerBottomPanel(...)` автоматически добавляют кнопку в Activity Bar, пункт меню «Вид» и вкладку. Управление — `show/hide/toggleSidePanel`, `showBottomPanel`, `unregisterBottomPanel`.
- `CommandConsole` (`ui/command_console.*`) — консоль команд: история ввода, выбор контекста, рендер `outputRequested` по mime-типу; в текущей оболочке это нижняя панель `console` (в старой `LauncherMainFrame` — отдельное окно с Ctrl + обратная кавычка).
- `PropertiesPanel` — свойства документа, следует за активным документом (на базе `PropertiesListWidget`).
- `WidgetRegistry` (`ui/widget_registry.*`) — реестр фабрик виджетов по строковому `type` (`WidgetFactory` + `WidgetTypeHandler`), задел под декларативный UI.
- Вспомогательные: `ActivityBar` (кнопки, позиции top/bottom, auto-hide), `SideHostPanel` (хост правых панелей), `DocumentTabsPanel` (вкладки, контекстное меню «Свойства»), `DocumentArea` (коллекция документов, `closeAll`), `BottomPanel` (сворачивание), `GlyphToolButton`, `NameSelectDialog`.

#### Окна MDI и команда windows (Legacy)

- `MdiChildWindow` (`Libs/Engine/mdi_child_window.*`) — наследник `QMdiSubWindow`: `windowType()`, `windowTitle()`, `windowId()`, `windowDefaultSizes()`, `handleCommand(instruction)`, `currentContext()`. Создаётся с `CommandContext` и id.
- `WindowsBuilder` (`src/Launcher/windows_builder.*`) — карта `target → lambda`: `user-login`, `user-profile`, `warmup`, `code-editor`, `fetch-api`, `fetch-api-queries-tree`.
- `WindowsController` (`src/Launcher/controllers/windows_controller.*`) — реестр окон по строковому id, поиск, активное окно, закрытие; сигналы `windowCreated`, `windowDestroyed`, `activeWindowChanged`; привязывается к `MdiArea`.
- `Controllers` — агрегатор `WindowsController` + `ActionPanelController`; доступ — `ApplicationController::controllers()`.
- Команда `windows` (`src/Launcher/commands/windows_cmd.*` и `commands/windows/*`): `action: create | close | list | invoke`, `target: <тип окна>`, `id: <id>` (по умолчанию UUID), `title: <заголовок>`. `create` строит окно, вызывает `handleCommand(instruction)`, добавляет его в `MdiArea` и регистрирует в контроллере.
- `MdiArea` (`Libs/BaseWidgets/mdi_area.*`) — `QMdiArea` с вкладками (document mode, закрытие, перемещение, история активации).

#### Конфигурации, темы и декларативный UI

- `bootstrap.yaml`: `appearance.theme`, `appearance.application.name`, `appearance.application.viewport.xtype`, `stores.items.<name>.wal.{checkInterval,maxSizeMb}`; парсится `BootstrapConfig::loadBootstrap()`, хранилища — в `Config::stores` (`Store`). Секция `stores` превращается в контексты `.store.<name>` через `BootstrapContextBuilder`.
- Тема: `assets/themes/{theme}/palette.yaml` (иерархия `QPalette`: `palette`, `base`, `colors.{Role}.{Active|Inactive|Disabled}.value` + `modifiers`) и `stylesheets/index.yaml` (список qss, например `base.qss`). `ThemeManager` объединяет qss и определяет схему, `PaletteParser` строит `QPalette`.
- `applications/{name}/main.yaml` — декларация UI приложения. Правила загрузки (`ClassManager`):
  - `define` — объявление класса виджета: `name` (полное имя), `extends` (базовый класс), `xtype` (псевдоним), `title`, `content`;
  - `requires` (массив) — подключаемые yaml-классы: путь строится по полному имени (`Common.Viewport.MapsForm` → `applications/{name}/Common/Viewport/MapsForm.yaml`), их `xtype` видны в текущем описании;
  - если файл базового/запрошенного класса не найден, имя считается нативным классом из `WidgetRegistry` (`Viewport.MainForm`, `console`, `explorer` и т.п.);
  - наследование: блоки дочернего класса перетирают базовые; мапы сливаются по ключам, последовательности — по идентичности элемента (ключ одноэлементной мапы, иначе `name`, иначе `xtype`), лишние элементы базы сохраняются;
  - `content` вьюпорта: блоки `bottombar`/`sidebar` с `default` и списком `content`; элемент панели — `xtype`, `name`, `glyph`, `title`, `tooltip`; панели создаются через `WidgetRegistry::build()` и регистрируются `MainWindow::registerSidePanel`/`registerBottomPanel`;
  - `ClassManager::createViewport()` строит вьюпорт по `appearance.application.viewport.xtype`; если описание не читается или класс не зарегистрирован, `Application` откатывается на `MainWindow` со встроенными панелями.
- `dataflow/*.yaml` и `Libs/Engine/scripting/dataflow/README.DataFlow.md` — спецификация DSL графов обработки данных (реестр процессоров, `entrypoints`, `nodes`/`inputs`/`outputs`, `policy`: executor/backpressure/priority). Реализации нет, `Configurations/dataflow/palette.yaml` пуст.

#### Точки расширения

1. Команда: наследник `ICommand` → регистрация в `ApplicationController::initContext()`.
2. Сервис/провайдер: интерфейс `I*` в модуле → фабрика в `ServicesFactory` (`src/Launcher/services.cpp`).
3. Окно: наследник `MdiChildWindow` → фабрика в `WindowsBuilder` → вызов через `windows action: create target: <тип>`.
4. Панель: `MainWindow::registerSidePanel` / `MainWindow::registerBottomPanel`.
5. Модуль: каталог `src/Content/{Module}` с целью `RedComplex<Module>` по образцу существующих → `add_subdirectory` в `src/Content/CMakeLists.txt` → линковка в `src/Launcher/CMakeLists.txt`.
6. Тема: каталог `assets/themes/{name}` с `palette.yaml` и `stylesheets/index.yaml`, имя — в `bootstrap.yaml`.
7. Подсветка/форматтер: наследник `HighlightingPlugin` или `FormatterPlugin`/`FormatterPluginAsync` → регистрация в `HighlightingPluginManager`/`FormatterPluginManager`.
8. Класс виджета для yaml: `WidgetsBuilder` (`src/Launcher/widgets_builder.cpp`) → `registry->registerFactory({className, xtype}, factory)`; фабрика получает `WidgetBuildHandler` (`name`, `title`, `glyph`, `parent`, `context`) и возвращает `QWidget*`.

#### Соглашения по слоям

- `Libs/Base`, `Libs/Resources`, `Libs/Graphics` не зависят от `Engine` и `Content`.
- `Engine` не знает о конкретных модулях: команды, сервисы и окна связываются в `Launcher`.
- Модуль предоставляет команды и сервисы, но не строит главное окно и не создаёт чужие сервисы.
- Межмодульное взаимодействие — через `ServicesRegistry` (типы) и `Instruction` (поведение), а не прямыми включениями чужих реализаций.

#### Переходные состояния и ограничения

- `src/Launcher/main_frame.*` (`LauncherMainFrame`) — старая оболочка на `MdiArea`, не используется текущим `main.cpp`; при доработке UI ориентируйтесь на `Libs/Engine/ui/main_window`.
- `src/Legacy/**` не входит в сборку — источник решений для возврата (инвентари, экипировка, карты, текстуры, материалы, персонажи, старые окна и виджеты).
- `Libs/DataFormat` и `Libs/DataStream` — legacy-слой чтения форматов Fallout 2 (`.dat`, `.frm`, `.pro`, `.int`, `.msg`, `.map`, `.pal` и др.); новый код на них не опирается, планируется вынос в отдельный модуль.
- `DatabasesService::reloadAliases()` сейчас не добавляет псевдонимы (вызовы `appendAlias` закомментированы), поэтому `connection("accounts")` вернёт ошибку «Undefined database alias»; псевдонимы нужно включать здесь.
- В `Application::Private::tryRun()` создаются отладочные контексты `test1`/`test2` — временный код проверки дерева контекстов.
- `DataFlow` — только спецификация; графы обработки ещё не исполняются. `ClassManager` применяет `content` только к `MainWindow` (`bottombar`/`sidebar`); вложенный `content` произвольных виджетов пока не разбирается.
- Команды пока принимают `ICommand::execute(instruction, context)`; запланирован переход на контекст-единственный интерфейс (параметры и состояние — в `CommandContext`). Отдельные контексты для документов/виджетов (например, свой контекст на копию `FetchApiWidget`) ещё не создаются — сейчас контекст создаётся на вызов команды.
- `BootstrapContextBuilder::reload()` идемпотентен: при повторном вызове переиспользует существующие контексты `.store.*` и обновляет значения переменных. Тесты не покрывают сценарий удаления хранилища из конфигурации (старые контексты останутся в реестре CommandController без владельца в builder, но их можно найти через `context(".store.<name")`); потребуется явный механизм очистки при необходимости.

#### Ключевые компоненты

**Запуск и команды**
- `Application` (`Libs/Engine/application.*`) — `run()`, настройка Qt/OpenGL, `setupResources()`, `registerTypes()`, создание главного окна; фатальные ошибки — `qFatal()`.
- `LauncherApplication` (`src/Launcher/main.cpp`) — метаданные приложения (`Hash2000` / `ru.hash2000` / `red-complex`) и сборка `ApplicationController` в `buildCommandController()`.
- `ApplicationController` (`src/Launcher/app_controller.*`) — реализация `CommandController`: `CommandProcessor`, `Controllers`, ленивая `ServicesRegistry`; `initContext()` регистрирует `help`, `echo`, `windows`, `users`, `fetch-api`.
- `CommandController` (`Libs/Engine/scripting/command_controller.*`) — глобальный контекст, `WidgetRegistry`, `Resources`, `execute*`, `print*`, сигналы `outputRequested`, `commandExecuted`, `commandFailed`.
- `CommandProcessor`, `CommandContext`, `Instruction`, `ICommand`, `ScriptProgram` — см. раздел «Команды, контексты и инструкции».

**Парсер**
- `SourceCursor`, `IParser`, `IAst`/`AstBase` (`scripting/parser/`).
- `parser/base/*`: `ParserValue`, `ParserNumber`, `ParserVariable`, `ParserAttributesCollection`, `ParserAttribute`, `ParserInclude`, `ParserBindings`, `ParserIdentifiersSpace`, `ParserPass`.
- `scripting/parser/README.md` — синтаксис DSL.

**Сервисы**
- `ServicesRegistry`, `IServicesFactory`/`LazyPtr` (`Libs/Engine/services/*`).
- `Services`/`ServicesFactory` (`src/Launcher/services.*`) — единственная точка регистрации сервисов и провайдеров.

**Ресурсы, профиль, БД**
- `Resources`, `VariablesContext`, `DirectoriesContext` (`Libs/Resources`).
- `DatabasesService` (`Content/DatabaseModule/services/databases_service.*`).
- `SQLiteConnection`, `SQLiteReader`, `MigrationManager`/`Migration`, `SQLiteWalManager` (`Libs/Resources/db/sqlite`).
- `AccountsMigrations`, `FetchApiMigrations`, `GameMigrations` (`Content/DatabaseModule/migrations`).
- `IDatabaseSettingsDataProvider` + `DatabaseSettingsDataProviderJsonImpl` (`Content/DatabaseModule/data_providers`).

**Bootstrap и темы**
- `BootstrapManager` (`scripting/bootstrap/bootstrap_manager.*`) — тема, палитра и `config()` (загруженный `bootstrap.yaml`); `BootstrapConfig::loadBootstrap()` (`bootstrap_config.h`).
- `BootstrapContextBuilder` (`scripting/bootstrap/bootstrap_context_builder.*`) — контексты `.store.<name>` с переменными WAL; владеет ими через `shared_ptr`.
- `ThemeManager`, `PaletteParser` (`Libs/Engine/themes/*`).

**Декларативный UI**
- `ClassManager` (`scripting/application/class_manager.*`) — загрузка `main.yaml`, разрешение `extends`/`requires`, слияние `content`, создание вьюпорта и панелей.
- `WidgetsBuilder` (`src/Launcher/widgets_builder.*`) — регистрация нативных классов: `Viewport.MainForm`/`mainform`, `ConsolePanel`/`console`, `PropertiesPanel`/`properties`, `ExplorerPanel`/`explorer`, `Panel`/`panel`.
- `WidgetRegistry` (`ui/widget_registry.*`) — `registerFactory(WidgetTypeHandler{className, xtype}, factory)` и `build(type, WidgetBuildHandler)`; `WidgetBuildHandler` передаёт `name`, `title`, `glyph`, `parent`, `context`.

**Оболочка**
- `MainWindow`, `ActivityBar`, `SideHostPanel`, `DocumentTabsPanel`, `DocumentArea`, `DocumentWidget`, `TextDocument`, `PropertiesPanel`, `BottomPanel`, `CommandConsole`, `WidgetRegistry`, `GlyphToolButton`, `NameSelectDialog` (`Libs/Engine/ui/*`).

**Окна MDI и контроллеры**
- `MdiChildWindow` (`Libs/Engine/mdi_child_window.*`), `MdiArea` (`Libs/BaseWidgets/mdi_area.*`).
- `WindowsController` (`src/Launcher/controllers/windows_controller.*`), `WindowsBuilder` (`src/Launcher/windows_builder.*`), `Controllers` (`src/Launcher/controllers.*`).
- `ActionPanelController`, `ActionButtonConfig` (`src/Launcher/controllers/action_panel_controller.*`, `action_button_config.h`).
- Команда `windows` и обработчики `windows_create`, `windows_close`, `windows_list`, `windows_invoke` (`src/Launcher/commands/windows/*`).

**Модули**
- ConsoleModule: `HelpCommand`, `EchoCommand` (`Content/ConsoleModule/commands`).
- DatabaseModule: `DatabasesService`, `DatabaseSettingsDataProviderJsonImpl`, миграции `accounts`/`fetch_api`/`game`.
- UsersModule: `UsersService`, `UsersRegistryService`, `UsersRegistryDataProviderDb`, `User`/`UserData`/`UserView`, `LoginWidget`, `RegisterWidget`, `UserWidget`, `crypto_extensions` (BIP-39-сид, соль, ключи БД).
- FetchApiModule: `FetchApiService`, `FetchStoreDbService`, `FetchTree`/`FetchQuery`/`FetchOpt`, `FetchApiWidget`, `FetchQueriesTreeWidget`, `FetchApiCommand`.
- MessagerModule: `MessagesDataProviderDb`, `IMessagesDataProvider`, `Message`.
- CodeEditorWidget: `CodeEditorWidget`, `Highlighter`, `HighlightingPlugin` + `HighlightingPluginManager` + плагины (bash, css, glsl, html, json, markdown, plain text), `FormatterPlugin`/`FormatterPluginAsync` + `FormatterPluginManager` + `ExternalCliFormatterPlugin`, `LanguageInfo`.
- Окна Launcher: `LoginWindow`, `UserWindow`, `WarmupWindow`/`WarmupWidget`, `CodeEditorWindow`, `FetchApiWindow`, `FetchApiQueriesTreeWindow` (`src/Launcher/windows/*`).

**Библиотеки**
- `Libs/Base`: `container_view.h` (`make_deref_view`), `exception.h`, `format.h`, `from.h`, `scoped_timer.h`, `crypto/security_buffer.h`, `crypto/bip_39.h`, `extensions/text_edit_extensions.h`.
- `Libs/BaseWidgets`: `MdiArea`, `PropertiesListWidget`, `KeyValueEditorWidget`, `FlowLayout`, `ClickableLabel`, lazy tree (`LazyTreeModel`, `LazyTreeView`, `LazyTreeWidget`, `LazyTreeBuilder`, `ILazyNodesDataProvider`, `LazyNodesDataProviderDb`, `HighlightDelegate`, `LazyTreeNodeRole`, `LazyTreeNodeState`).
- `Libs/Graphics`: `BaseOpenGLWidget`, `Camera`, `ShaderProgram`, `UniformBuffer` + `FrameUniformBuffer`/`ViewMatricesUniformBuffer`, `TextureAtlas`, `UploadedTexture`, `TileRenderer`, `Chunk` + `ChunkDrawTileset`/`ChunkDrawBorder`/`ChunkDrawData`, `TextureRegion`, `TileData`, `PixmapExtensions`.
- `Libs/DataStream`: `DataStream`, `DataWriteStream`, `DataStreamBuffer`/`DataStreamFile`/`DataStreamContainer`, `DatFile`, `RawDirectory`, `FileReader`.
- `Libs/DataFormat`: `data_format/{bio,frm,gam,gcd,int,json,map,msg,pal,pixmap,pro,sve,txt}/data_reader` и `proto/*` — модели и константы форматов Fallout 2.
- `src/externals/FastNoiseLite` — header-only генератор шума, не из vcpkg.

**Вывод в консоль**
- `ConsoleTable`, `ConsoleJson`, `ConsoleImage` (`Libs/Engine/console/*`); mime-константы `kCommandPrintStyle_Plane`/`kCommandPrintStyle_Table`/`kCommandPrintStyle_Json` (`command_controller.h`); классы стилей `info`, `error`, `success`, `warning`, `system`.

**Цели сборки и тесты**
- Цели: `Launcher`, `RedComplexBase`, `RedComplexBaseWidgets`, `RedComplexResources`, `RedComplexGraphics`, `RedComplexEngine`, `RedComplexDataStream`, `RedComplexDataFormat`, `RedComplexConsoleModule`, `RedComplexDatabaseModule`, `RedComplexUsersModule`, `RedComplexFetchApiModule`, `RedComplexMessagerModule`, `RedComplexCodeEditorWidget`.
- Тесты: `TestEngine` (парсер, контексты, палитра — `tests/engine`), `TestItemsService` и `TestInventoriesService` (заготовки), агрегат `run_tests`; отдельной цели `RunTests` в `tests/CMakeLists.txt` нет.


## Ссылки на существующие скилы


## Рабочий процесс разработки

1. **Создание ветки функции**: `git checkout -b feature/your-feature`
2. **Внесение изменений** в соответствии со стандартами кода выше
3. **Сборка и тестирование** — Использовать Qt Test Framework для автоматизированного тестирования
4. **Коммит с понятными сообщениями**: `git commit -m "Description of changes"`
5. **Пуш и создание PR** когда готово

---
