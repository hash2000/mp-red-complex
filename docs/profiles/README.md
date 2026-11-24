# Профили пользователей

Профиль — это каталог с настройками текущего пользователя в рабочей копии
конфигурации:

```text
<path.profile>/profiles/<имя>/
  appearance.yaml        оформление и описываемое приложение
```

`path.profile` — каталог, который приложение читает при запуске
(`%LOCALAPPDATA%\Hash2000\red-complex` в переносимой сборке). Каталог
`profiles` создаётся автоматически; в эталонной конфигурации
(`src/Configurations`) его нет, поэтому `RedComplex.cmd /update-config` не
перезаписывает пользовательские настройки.

## Профиль default

Пока пользователь не вошёл в систему, работает профиль `default`:

- при первом запуске `Resources::load()` создаёт `profiles/default`;
- `BootstrapManager::reload()` читает `bootstrap.yaml` и передаёт его блок
  `appearance` как значения по умолчанию;
- если `profiles/default/appearance.yaml` ещё нет (или он пуст), файл
  создаётся из этих значений;
- при следующих запусках действует уже `appearance.yaml` профиля, а
  `bootstrap.yaml` остаётся только шаблоном для новых профилей.

Поэтому оформление можно менять в `profiles/default/appearance.yaml` вручную,
не трогая описание приложения. Чтобы вернуть значения из `bootstrap.yaml`,
достаточно удалить файл настроек профиля.

```yaml
theme: dark
application:
  name: base
  viewport:
    xtype: BaseViewport
```

Неизвестные и отсутствующие ключи не ломают запуск: недостающие значения
берутся из `bootstrap.yaml`. Синтаксическая ошибка YAML или неверный тип
`theme`/`application` считаются ошибкой конфигурации профиля.

## Доступ к базам данных

Профилю неавторизованного пользователя доступны только открытые
(незашифрованные) хранилища:

- `Resources::allowsEncryptedStores()` возвращает `false`, пока выбран
  профиль `default`;
- `DatabasesService::connection()` и `verifyDatabaseFile()` отказывают в
  зашифрованной базе, даже если ключ шифрования уже установлен;
- `DatabaseBackupService` не показывает зашифрованные хранилища в списке и не
  выполняет с ними операции.

После входа в систему `Resources::loadProfile(userHash)` выбирает
`profiles/<user hash>` и разрешает зашифрованные хранилища; выход возвращает
профиль `default`. Применение оформления профиля на лету и экран настроек —
следующий шаг; сейчас настройки профиля читаются при старте приложения.

## Файлы

- `src/Libs/Engine/profiles/profile.*` — чтение, инициализация и сохранение
  `appearance.yaml`;
- `src/Libs/Resources/profiles/profile_name.*` — имя `default` и проверка
  допустимых имён профилей;
- `src/Libs/Resources/resources.*` — `path.profiles`, `path.profiles.current`,
  `currentProfileName()`, `selectProfile()`, `allowsEncryptedStores()`;
- `src/Libs/Engine/scripting/bootstrap/bootstrap_manager.*` — применение
  профиля при запуске.

## Тесты

- `tests/engine/test_profile.cpp` — инициализация пустого профиля, перекрытие
  значений по умолчанию, ошибки настроек, выбор каталога и права профиля;
- `src/Content/DatabaseModule/tests/test_databases_service.cpp` — запрет
  зашифрованной базы в профиле `default`;
- `src/Content/DatabaseModule/tests/test_database_backup.cpp` — скрытие
  зашифрованного хранилища в списке резервного копирования.
