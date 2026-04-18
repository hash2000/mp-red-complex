---
name: create-mdi-window
description: Create a new MDI child window following the project's layered architecture pattern
---

# Creating New MDI Windows

To add a new MDI child window, follow this established pattern across Data Layer → Application Layer → UI Layer → Registration.

## 1. Create Data Layer Interface

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

## 2. Create Data Layer Implementation

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

## 3. Create Application Layer Service

```cpp
// src/ApplicationLayer/your_module/your_service.h
#pragma once
#include "DataLayer/your_module/i_your_data_provider.h"
#include <QObject>
#include <memory>

class YourService : public QObject {
    Q_OBJECT
public:
    explicit YourService(IYourDataProvider* dataProvider, QObject* parent = nullptr);
    ~YourService() override;

    void doSomething();

signals:
    void dataChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};
```

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
    d->dataProvider = dataProvider;
}
```

**Important: Service-Provider Ownership Pattern**

1. `Services` class owns data providers via `std::unique_ptr<IYourDataProvider>`
2. Services receive non-owning raw pointers
3. Ensures clear ownership, no double-deletion, easy testing

```cpp
// ❌ WRONG: Service takes ownership
d->yourService = std::make_unique<YourService>(
    std::make_unique<YourDataProviderJsonImpl>(resources));

// ✅ CORRECT: Services owns provider, service gets pointer
d->yourDataProvider = std::make_unique<YourDataProviderJsonImpl>(resources);
d->yourService = std::make_unique<YourService>(d->yourDataProvider.get());
```

## 4. Create Widget (UI Component)

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

## 5. Create MDI Window

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

## 6. Register Service in `services.cpp`

```cpp
// src/Game/services.h - add forward declaration
class YourService;
class IYourDataProvider;

// src/Game/services.h - add method
YourService* yourService() const;

// src/Game/services.cpp - add includes
#include "ApplicationLayer/your_module/your_service.h"
#include "DataLayer/your_module/your_data_provider_json_impl.h"
#include "DataLayer/your_module/i_your_data_provider.h"

// src/Game/services.cpp - add to Private class
std::unique_ptr<YourService> yourService;
std::unique_ptr<IYourDataProvider> yourDataProvider;

// src/Game/services.cpp - create in constructor
d->yourDataProvider = std::make_unique<YourDataProviderJsonImpl>(resources);
d->yourService = std::make_unique<YourService>(d->yourDataProvider.get());

// src/Game/services.cpp - add accessor
YourService* Services::yourService() const {
    return d->yourService.get();
}
```

## 7. Register Window in `windows_builder.cpp`

```cpp
#include "Game/widgets/your_module/your_window.h"

// In constructor:
d->factory.emplace("your-window-type", [](Services* services, const QString& id) {
    return new YourWindow(
        services->yourService(),
        id);
});
```

## 8. Create Window via Command

```cpp
// Via code:
controller->executeCommandByName("window-create", QStringList{ "your-window-type", "unique-id" });

// Via console:
window-create your-window-type unique-id
```
