# Контекст проекта RedComplex для SourceCraft Code Assistant

RedComplex — это пет-проект на C++ с использованием фреймворка Qt, представляющий собой оконное приложение с элементами игры. Основной акцент сделан на систему инвентаря и экипировки. Проект использует современные подходы C++20, включая умные указатели, сигналы/слоты Qt, и модульную архитектуру.

Этот файл предоставляет контекст для SourceCraft Code Assistant, чтобы помочь агенту понимать структуру проекта, стандарты кода, архитектуру и рабочие процессы.

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
cmake --build build --target ResourcesTool
cmake --build build --target Game
cmake --build build --target Launcher
```

### Запуск приложений
```bash
# Запуск ResourcesTool
./build/src/ResourcesTool/ResourcesTool

# Запуск Game
./build/src/Game/Game

# Запуск Launcher
./build/src/Launcher/Launcher
```

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

class Foo : public QObject {
Q_OBJECT
public:
    Foo(QObject* parent = nullptr);
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
, d(std::make_unique<Private>(this)) {}
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
```
src/
├── Base/           # Базовые утилиты и виджеты
├── BaseWidgets/    # Переиспользуемые UI компоненты
├── ApplicationLayer/ # Оркестрация приложения
├── Content/        # Редакторы контента (Map, Shader, Particles)
├── DataFormat/     # Определения форматов данных. Legacy.
├── DataLayer/      # Слой доступа к данным
├── DataStream/     # Потоковый ввод/вывод (DAT, RAW форматы).
├── Engine/         # Ядро движка
├── Game/           # Основное игровое приложение
├── Launcher/       # Приложение-лаунчер. Legacy.
├── Resources/      # Управление ресурсами
├── ResourcesTool/  # Инструмент просмотра/редактирования ресурсов. Legacy.
└── tests/          # Модульные тесты (Qt Test Framework)
```

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

#### Основной поток приложения
1. **Слой Engine**: Приложение начинается в `main.cpp`, где `GameApplication` наследуется от `Application`. Этот базовый класс обрабатывает базовую инициализацию, такую как приложение Qt, загрузку конфигурации и настройку ресурсов.
2. **Создание главного окна**: `Application::run()` вызывает `createMainFrame()` (чисто виртуальный). В `GameApplication` это возвращает `GameMainFrame`.
3. **Слой Application**: `GameApplication` создаётся первым, затем он создаёт `ApplicationController` в приватном классе `GameMainFrame`. Этот контроллер оркестрирует высокоуровневую логику приложения.

#### Ключевые компоненты

**ApplicationController** (`src/Game/app_controller.*`)
- Центральный координатор. Владеет и управляет:
  - `CommandProcessor`: Разбирает и выполняет текстовые команды.
  - `Services`: Содержит основные игровые сервисы (время, мир, инвентарь, предметы).
  - `Controllers`: Управляет UI контроллерами (например, `WindowsController`).
- Использует паттерн PIMPL с приватным классом `Private` для инкапсуляции.

**Services** (`src/Game/services.*`)
- Контейнер для всех основных игровых систем.
- Ключевые сервисы включают:
  - `TimeService`: Точный таймер для игровых тиков и запланированных событий.
  - `InventoriesService`: Основная логика управления инвентарями и экипировкой.
  - `ItemsService`: Управляет определениями предметов и их созданием.
- Инициализируются с провайдерами данных (например, `ItemsDataProviderJsonImpl`) для загрузки игровых данных из JSON файлов.

**Система инвентаря и экипировки**
- **Слой данных** (`src/DataLayer`): Обеспечивает доступ к сырым данным.
  - `ItemsDataProviderJsonImpl`: Загружает определения предметов (`items.json`, `items_ids.json`) и иконки из папки assets.
  - `InventoryDataProviderJsonImpl` / `EquipmentDataProviderJsonImpl`: Загружают состояние инвентаря и экипировки из JSON файлов в папке data.
  - `InventoriesDataProviderJsonImpl`: Обнаруживает, какие файлы инвентаря и экипировки существуют.
- **Слой приложения** (`src/ApplicationLayer`): Реализует бизнес-логику.
  - `ItemsService`: Загружает все сущности предметов и может создавать экземпляры (метод `duplicate`).
  - `InventoriesService`: Центральный хаб. Использует `ItemPlacementStore` для загрузки экземпляров `IItemPlacementService` для конкретных инвентарей или экипировки.
    - `InventoryStoreImpl` -> `InventoryService`: Для обычных инвентарей.
    - `EquipmentStoreImpl` -> `EquipmentService`: Для слотов экипировки.
  - `ItemMimeData`: Объект передачи данных, используемый для операций drag-and-drop между контейнерами.
  - `IItemPlacementService`: Абстрактный интерфейс (`pure virtual`), определяющий операции типа `canPlaceItem`, `moveItem`, `applyDublicateFromItem`. И `InventoryService`, и `EquipmentService` реализуют его.
- Эта конструкция позволяет `InventoriesService` обрабатывать перемещения между любыми двумя контейнерами (инвентарь-инвентарь, инвентарь-экипировка), рассматривая их все как экземпляры `IItemPlacementService`.

**Управление стеками предметов (Пачки предметов)**
- Предметы могут складываться, если `entity->maxStack > 1`
- При перемещении **полного стека** между инвентарями ID предмета сохраняется (дубликат не создаётся)
- При перемещении **частичного стека** создаётся новый предмет с новым ID для перемещённого количества
- `InventoryService::transferItem()` — Переместить/добавить предмет с сохранением ID
- `InventoriesService::moveItem()` — Автоматически обрабатывает логику полного/частичного стека

**Виджет создания предметов**
- `ItemCreateWidget` (`src/Game/widgets/items/item_create_widget.*`) — Диалог для создания новых предметов
- Позволяет выбрать количество (от 1 до maxStack)
- Создаёт предметы в последнем активном окне инвентаря
- Использует `ItemsService::createItemByEntity()` для создания экземпляров предметов из определений сущностей

**Система сохранения предметов**
- `ItemsSaveManager` (`src/ApplicationLayer/items_save_manager.*`) — Сохраняет все созданные предметы
- `ItemsDataWriterJsonImpl` — JSON writer для сохранения предметов
- Предметы сохраняются в `data/items/{uuid}.json`
- Подключён к сигналу `Services::save` вместе с сохранением инвентаря/экипировки

---

## Ссылки на существующие скилы

В проекте уже определены скилы для Qwen, расположенные в `.qwen/skills/`. Они могут быть полезны и для SourceCraft Code Assistant:

- **/create-mdi-window** — Полная пошаговая инструкция по созданию нового MDI окна (Data Layer → Application Layer → UI Layer → Registration)
- **/widget-styling** — Цветовая палитра Tailwind Slate, стили компонентов, layout guidelines
- **/resources-io** — Чтение и запись файлов через Resources system (assets/data контейнеры)
- **/commit** — Stage, commit, and push changes with conventional commit message
- **/test** — Инструкции по тестированию

## Рабочий процесс разработки

1. **Создание ветки функции**: `git checkout -b feature/your-feature`
2. **Внесение изменений** в соответствии со стандартами кода выше
3. **Сборка и тестирование** — Использовать Qt Test Framework для автоматизированного тестирования
4. **Коммит с понятными сообщениями**: `git commit -m "Description of changes"`
5. **Пуш и создание PR** когда готово

---

## Примечания для SourceCraft Code Assistant

- При работе с проектом учитывайте, что это C++/Qt приложение с акцентом на инвентарь и экипировку.
- Используйте существующие скилы как参考 для выполнения типовых задач.
- При создании новых файлов следуйте соглашениям об именовании и структуре проекта.
- Для отладки используйте логирование Qt (`qDebug`, `qWarning` и т.д.).
- Тесты находятся в `tests/`, используйте Qt Test Framework для добавления новых тестов.

Этот файл должен регулярно обновляться по мере изменения проекта.
