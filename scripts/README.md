# Скрипты запуска тестов RedComplex

Каталог содержит обёртки над CMake и CTest для Windows. Все скрипты работают с тем
же каталогом сборки, что и `CMakeSettings.json`, и не создают отдельную конфигурацию.

## Требования

- Windows и Windows PowerShell 5.1 или новее.
- CMake 4.1+ в `PATH` (`cmake`, `ctest`).
- Сконфигурированный каталог сборки, по умолчанию `build\x64-Clang-Debug`.
- Visual Studio с clang-cl. Окружение Visual Studio (`INCLUDE`, `LIB`, `PATH`)
  подхватывается автоматически через `VsDevCmd.bat`; если оно уже настроено,
  передайте `-NoVsEnvironment`.
- Файлы `.ps1` сохранены в UTF-8 с BOM: Windows PowerShell 5.1 иначе неверно
  читает русские строки сообщений.

## Быстрый старт

Из корня репозитория:

```powershell
# Собрать и запустить все тесты
.\scripts\run-tests.ps1

# Только UI-тесты
.\scripts\run-tests.ps1 -Suite ui

# Параллельный запуск и JUnit-отчёт
.\scripts\run-tests.ps1 -Parallel 8 -JUnit build\test-results.xml

# Один тест целиком
.\scripts\run-test.ps1 TestEngine

# Одна тестовая функция
.\scripts\run-test.ps1 -Name TestEngine -TestFunction testContext_VariablesLazyMode

# Список наборов и тестов
.\scripts\list-tests.ps1

# Список наборов вместе с функциями Qt Test
.\scripts\list-tests.ps1 -Functions

# Только сборка тестов, без запуска
.\scripts\build-tests.ps1 -Suite database
```

Если PowerShell блокирует выполнение сценариев:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

## Наборы тестов

| Набор | Состав |
|-------|--------|
| `services` | `TestItemsService`, `TestInventoriesService` |
| `engine` | `TestEngine` |
| `database` | `TestDatabasesService`, `TestLazyNodesDataProviderDb` |
| `system-router` | `TestSystemRouter`, `TestSystemRouterCommandsDb` |
| `ui` | `TestUiZones`, `TestDocumentTabs`, `TestPropertiesPanel`, `TestClassManager`, `TestHighlighters`, `TestCodeEditorSearch` |
| `basewidgets` | `TestLazyTreeView`, `TestLazyTreeWidget` |
| `all` | Объединение всех наборов (по умолчанию) |

## run-tests.ps1

Собирает цели выбранного набора и запускает соответствующие тесты через `ctest`.

| Параметр | По умолчанию | Назначение |
|----------|--------------|------------|
| `-Suite` | `all` | Имя набора из таблицы выше. |
| `-BuildDirectory` | `build\x64-Clang-Debug` | Каталог сборки. Относительный путь считается от корня репозитория. Если штатного каталога нет, скрипт ищет единственный `build\*\CMakeCache.txt`. |
| `-NoBuild` | выключен | Не собирать тесты, запустить уже собранные. |
| `-Parallel` | `1` | Число параллельных процессов `ctest --parallel`. Сборка всегда выполняется с `--parallel`. |
| `-Platform` | не задан | Значение `QT_QPA_PLATFORM`, например `offscreen` для headless-запуска. |
| `-JUnit` | не задан | Путь XML-отчёта `ctest --output-junit`; каталог создаётся автоматически. |
| `-CtestArguments` | пусто | Дополнительные аргументы ctest массивом, например `'--repeat', 'until-fail:3'`. |
| `-NoVsEnvironment` | выключен | Не инициализировать окружение Visual Studio. |

Скрипт вызывает ctest с точным фильтром `-R '^(TestA|TestB)$'` и `--no-tests=error`,
поэтому пропавшее или переименованное имя теста сразу считается ошибкой.

## run-test.ps1

Собирает одну цель и запускает её как обычный исполняемый файл Qt Test из каталога
сборки. Рабочий каталог процесса — каталог сборки.

| Параметр | По умолчанию | Назначение |
|----------|--------------|------------|
| `-Name` | обязателен | Имя цели CMake без `.exe`, например `TestEngine`. Позиционный параметр. |
| `-TestFunction` | не задан | Имя функции Qt Test; передаётся позиционным аргументом. |
| `-ListFunctions` | выключен | Вывести функции теста (`-functions`) и не запускать тесты. |
| `-QtArguments` | пусто | Дополнительные аргументы Qt Test массивом, например `'-v2'`. |
| `-NoBuild` | выключен | Не собирать цель. |
| `-BuildDirectory` | `build\x64-Clang-Debug` | Каталог сборки. |
| `-Platform` | не задан | Значение `QT_QPA_PLATFORM`. |
| `-NoVsEnvironment` | выключен | Не инициализировать окружение Visual Studio. |

## build-tests.ps1

Собирает тесты выбранного набора и не запускает их. Параметры: `-Suite`,
`-BuildDirectory`, `-Parallel` (передаётся в `cmake --build --parallel`),
`-NoVsEnvironment`.

## list-tests.ps1

Показывает наборы, цели, признак сборки и, с `-Functions`, функции Qt Test.
Параметры: `-Suite`, `-BuildDirectory`, `-Functions`, `-Platform`.
Скрипт ничего не собирает; для отсутствующих тестов выводится «не собран».

## Окружение запуска

- `QT_PLUGIN_PATH` задаётся автоматически, если он ещё не задан: берётся
  `vcpkg_installed\<triplet>\debug\Qt6\plugins` (затем `release` и общий вариант).
- `QT_QPA_PLATFORM` меняется только при явном `-Platform`.
- `PATH` и переменные Visual Studio меняются только на время сборки в текущем процессе.

## Добавление нового теста

1. Добавьте цель и `add_test` в CMake, как у существующих тестов.
2. Добавьте имя цели в подходящий набор в `scripts\_common.ps1`, чтобы новый тест
   попал в `all` и в `run-tests.ps1 -Suite <набор>`.
3. `scripts\run-test.ps1 <ИмяЦели>` работает с новой целью без правки скриптов.

## Коды возврата

Скрипты возвращают код сборки или тестов: `0` — успех, ненулевое значение — ошибка.
`run-tests.ps1` дополнительно печатает итоговое сообщение, но не скрывает вывод ctest.

## Ограничения

- CTest пишет `Testing\Temporary\LastTest.log`, поэтому каталог сборки должен быть
  доступен для записи.
- GUI-тесты на headless-агенте запускайте с `-Platform offscreen`.
- Скрипты рассчитаны на Windows и Visual Studio; для Linux используйте `ctest`
  напрямую.
