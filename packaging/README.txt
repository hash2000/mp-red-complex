RedComplex — переносимая сборка для Windows x64
================================================

Запуск
------
Дважды щёлкните RedComplex.cmd (или запустите его из консоли).

При первом запуске эталонная конфигурация из папки Configurations копируется в
профиль пользователя:

    %LOCALAPPDATA%\Hash2000\red-complex

Дальше приложение работает с этой рабочей копией: её можно менять, и при
следующих запусках она не перезаписывается.

Вернуть эталонные описания приложения, тему и список миграций из этой сборки:

    RedComplex.cmd /update-config

Базы данных и пользовательские файлы в
%LOCALAPPDATA%\Hash2000\red-complex\data при этом не затрагиваются.

Состав архива
-------------
Launcher.exe        приложение
*.dll               Qt 6 и зависимости: OpenSSL, SQLCipher, libsodium, yaml-cpp, ICU
platforms\, tls\,   плагины Qt (платформа Windows, стили, HTTPS, форматы изображений)
styles\ и другие
Configurations\     эталонная конфигурация: bootstrap.yaml, applications, assets,
                    stores, dataflow
version.txt         версия сборки
LICENSE             лицензия проекта

Требования
----------
Windows 10/11 x64.

Если приложение не запускается из-за отсутствующих системных библиотек,
установите Visual C++ Redistributable 2015-2022 (x64) с сайта Microsoft.

Лицензии
--------
Проект распространяется по лицензии из файла LICENSE.

Qt 6 используется по лицензии LGPLv3:
https://www.qt.io/licensing/open-source-lgpl-obligations
