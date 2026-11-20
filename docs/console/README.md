# Консоль и команды

Консоль — нижняя панель `console`. Она принимает текст команды, передаёт его
в `CommandProcessor`, а результат отображает по mime-типу (`text/plain`,
`application/table`, `application/json`).

Основные классы:

- `CommandController` — `src/Libs/Engine/scripting/command_controller.h`;
- `CommandProcessor` — `src/Libs/Engine/scripting/command_processor.h`;
- `CommandContext` — `src/Libs/Engine/scripting/command_context.h`;
- `ICommand` — `src/Libs/Engine/scripting/i_command.h`.

## 1. Синтаксис строки

```text
command param=value, param2="текст с пробелами", flag=true
```

- имя команды и параметра — буква или `_`, далее буквы, цифры, `_`, `.`, `-`;
- параметры разделяются запятой;
- обычное значение без кавычек читается до пробела или запятой и считается
  строкой с mime-типом `text/plane` (историческое написание в парсере);
- значение в двойных кавычках может содержать пробелы; `\` экранирует символ
  внутри строки;
- числа, `true`/`false` разбираются как значения;
- блоки:

```text
<<text>>
многострочный текст
<</text>>
```

```text
<<json>>{"name":"value"}<</json>>
<<xml>><root/><</xml>>
<<ini>>key=value<</ini>>
```

Mime-типы блоков: `text/plain`, `application/json`, `text/xml`,
`text/x-ini`.

Пример:

```text
echo name="Привет", count=3, enabled=true
fetch-api action=get, location="https://example.com", show-editor=true
```

## 2. Контекст команды

`CommandController::execute(text)` создаёт временный локальный контекст, а
`CommandProcessor` создаёт от него контекст команды:

- имя контекста — имя команды;
- режим — `Mutable`: команда видит переменные вызывающей стороны и может
  изменять их;
- параметры командной строки записываются как **собственные** переменные
  команды, поэтому они не затирают переменные базового контекста;
- контекст команды не регистрируется в глобальном дереве и живёт только на
  время вызова.

Внутри команды параметры читаются так:

```cpp
const auto action = context->variable("action").toString();
const auto count = context->variable("count", 0).toInt();
const auto hasName = context->hasVariable("name");
```

Для mime-типа параметра:

```cpp
const auto mime = context->variableMimeType("name");
```

`ownVariables()` содержит параметры, переданные при вызове; `variables()` —
все видимые переменные, включая унаследованные.

## 3. `ICommand`

```cpp
class MyCommand final : public ICommand {
public:
    explicit MyCommand(QObject* parent = nullptr)
        : ICommand(parent) {
    }

    QString name() const override { return "my-command"; }
    QString description() const override { return "Краткое описание"; }
    QString help() const override {
        return "my-command name=<text>";
    }

    int minArgs() const override { return 1; }
    int maxArgs() const override { return 1; }

    bool execute(std::shared_ptr<CommandContext> context) override {
        auto* controller = context->commandController();
        const auto name = context->variable("name").toString();
        if (name.isEmpty()) {
            controller->printError("Не задано имя");
            return false;
        }

        controller->printSuccess(QString("Привет, %1").arg(name));
        return true;
    }
};
```

`minArgs()` / `maxArgs()` считают параметры без имени команды; `-1` у
`maxArgs()` означает отсутствие ограничения. Возврат `false` означает
неуспешное выполнение.

Регистрация выполняется в `ApplicationController::initContext()`:

```cpp
commands->registerCommand(std::make_unique<MyCommand>(this));
```

Имена команд приводятся к нижнему регистру; повторная регистрация отклоняется
с предупреждением.

## 4. Вывод

Используйте методы `CommandController`:

```cpp
controller->print("обычный текст");
controller->printSystem("системное сообщение");
controller->printSuccess("успех");
controller->printWarning("предупреждение");
controller->printError("ошибка");
```

Таблицы:

```cpp
ConsoleTable table({ "id", "name" });
table.addRow({ 1, "Первый" });
table.setColumnAlignment("id", ConsoleTable::Right);
controller->print(table);
```

`print()` можно вызывать из любого потока: `outputRequested` доставляется
через `Qt::QueuedConnection`. Поэтому асинхронные операции (fetch, загрузка
файла) тоже могут писать в консоль.

Правила:

- ошибки и предупреждения — только через `printError()` / `printWarning()`;
- не используйте `QMessageBox` для ошибок;
- диалог допустим, только если нужно спросить пользователя (выбор файла,
  подтверждение удаления);
- таблица должна иметь осмысленные заголовки.

## 5. Проверка и справка

```text
help
help name=documents
help name=system-router-routes
```

`help` без параметров выводит список команд и глобальных переменных;
`help name=<команда|переменная>` — справку по одному элементу.

## 6. Встроенные команды

Полный список с параметрами и примерами — в [commands.md](commands.md).

Кратко:

- `help`, `echo`;
- `documents`, `info-panels`, `side-panels` — управление зонами;
- `users` — вход, выход, регистрация, восстановление;
- `fetch-api` — HTTP-запросы;
- `system-router-save-blob` — сохранение содержимого блока из code-editor;
- `system-router-blobs`, `system-router-routes` — таблицы блоков и узлов.

## См. также

- [Команды консоли](commands.md)
- [Контексты](../contexts/README.md)
- [Документы](../documents/README.md)
