---
name: resources-io
description: Read and write files through the Resources system (data/assets containers)
---

# Работа с Resources — чтение и запись данных

Проект использует систему `Resources` для доступа к файлам. Пути `data/` и `assets/` — это контейнеры верхнего уровня в базовой директории из `Resources.Path`.

**Никогда не используйте `QFile` напрямую для чтения** — только через `Format::*::DataReader`. Запись в `data/` допустима через `QFile` + `Resources.Path`.

## Получение базового пути

```cpp
auto dataDir = resources->Variables.get("Resources.Path", "").toString();
if (dataDir.isEmpty()) {
    qWarning() << "Resources.Path not found";
    return false;
}
```

## Чтение JSON из `assets`

```cpp
#include "DataStream/format/json/data_reader.h"
#include "Resources/resources.h"

QJsonObject json;
Format::Json::DataReader reader(resources, "assets", "items/entities_ids.json");
if (!reader.read(json)) {
    return false;
}

const QJsonArray items = json["items"].toArray();
```

## Чтение JSON из `data`

```cpp
QJsonObject json;
Format::Json::DataReader reader(resources, "data", "tile_groups/ground.json");
if (!reader.read(json)) {
    return std::nullopt;
}
```

## Чтение изображения (Pixmap)

```cpp
#include "DataStream/format/pixmap/data_reader.h"

QPixmap pixmap;
Format::Pixmap::DataReader reader(resources, "assets", "textures/tiles/ground.png");
if (!reader.read(pixmap)) {
    return std::nullopt;
}
```

## Перечисление файлов в контейнере

```cpp
QStringList result;
const QString prefix = "icons/";

for (const auto& container : resources->items()) {
    const auto items = container.items();
    for (const auto& [path, stream] : items) {
        if (stream->containerName() == "assets"
            && path.startsWith(prefix)
            && path.endsWith(".png", Qt::CaseInsensitive)) {
            const QString fileName = path.mid(prefix.length());
            result.append(fileName);
        }
    }
}
result.sort();
```

## Запись JSON в `data`

Для записи используется `QFile` с базовым путём:

```cpp
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <qDebug>

auto dataDir = resources->Variables.get("Resources.Path", "").toString();
QDir dir(dataDir);
dir.mkpath("data/tile_groups");

QFile file(dir.filePath("data/tile_groups/ground.json"));
if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    qWarning() << "Can't open file:" << file.fileName();
    return false;
}

file.write(doc.toJson());
return true;
```

## Удаление файла из `data`

```cpp
auto dataDir = resources->Variables.get("Resources.Path", "").toString();
QDir dir(dataDir);
return QFile::remove(dir.filePath("data/tile_groups/ground.json"));
```

## Сканирование директории в `data`

```cpp
auto dataDir = resources->Variables.get("Resources.Path", "").toString();
QDir dir(dataDir + "/data/tile_groups");
if (!dir.exists()) return false;

const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
for (const auto& entry : entries) {
    if (!entry.filePath().endsWith(".json")) continue;
    // Обработка
}
```
