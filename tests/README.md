# Тесты для RedComplex

## Запуск тестов

### Сборка и запуск всех тестов

```bash
# Конфигурация
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Сборка таргета тестов
cmake --build build --target RunTests

# Запуск тестов
cmake --build build --target run_tests

# Или напрямую через ctest
cd build
ctest --output-on-failure
```

### Запуск отдельных тестов

```bash
# Запуск конкретного тестового файла
./build/tests/RunTests

# Запуск тестов по имени (фильтрация)
./build/tests/RunTests -testfunction testLoadEntities
```

## Структура тестов

```
tests/
├── CMakeLists.txt              # Конфигурация сборки тестов
├── mock_items_data_provider.*  # Моковые данные для тестов
└── test_items_service.cpp      # Тесты для ItemsService
```

## Написание тестов

### Базовый шаблон теста

```cpp
#include <QtTest/QtTest>
#include "ApplicationLayer/items/items_service.h"

class TestMyClass : public QObject {
    Q_OBJECT

private slots:
    void init();              // Перед каждым тестом
    void cleanup();           // После каждого теста
    void testMyFunction();    // Сам тест
};

void TestMyClass::testMyFunction() {
    // Arrange
    ItemsService service(dataProvider);
    
    // Act
    const auto* item = service.duplicate("item-id");
    
    // Assert
    QVERIFY(item != nullptr);
    QCOMPARE(item->entityId, QString("item-id"));
}

QTEST_MAIN(TestMyClass)
#include "test_myclass.moc"
```

### Основные макросы проверок

| Макрос | Описание |
|--------|----------|
| `QVERIFY(condition)` | Проверка истинности условия |
| `QCOMPARE(actual, expected)` | Сравнение значений |
| `QTEST_MAIN(TestClass)` | Точка входа для тестов |
| `QSKIP("reason")` | Пропуск теста |
| `QFAIL("message")` | Принудительный провал |

### Моковые данные

Используйте `MockItemsDataProvider` для создания тестовых данных:

```cpp
MockItemsDataProvider dataProvider;

// Добавление сущности
ItemEntity entity;
entity.id = "test-item";
entity.name = "Test Item";
dataProvider.addEntity("test-item", entity);

// Добавление предмета
Item item;
item.id = "item-uuid";
item.entityId = "test-item";
dataProvider.addItem("item-uuid", item);

// Передача в сервис
ItemsService service(&dataProvider);
```

## Добавление новых тестов

1. Создайте файл `test_<component>.cpp` в папке `tests/`
2. Добавьте файл в `tests/CMakeLists.txt`
3. Напишите тесты по шаблону выше
4. Запустите `cmake --build build --target RunTests`

## Интеграция с CI

Тесты выводят результаты в формате XML:

```bash
ctest --output-on-failure --output-junit results.xml
```
