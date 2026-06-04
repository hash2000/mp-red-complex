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

**Important: Lazy Dependency Injection with ThreadSafeLazyPtr**

1. `Services::Private` owns all components via `LazyPtr<T>` (thread-safe lazy wrapper over `std::unique_ptr`)
2. Components receive non-owning raw pointers to dependencies via `.get()` calls inside factory lambdas
3. Dependencies are instantiated on first access (lazy initialization), ensuring:
   - Minimal startup time and resource usage
   - Clear ownership: `Services` is the single owner, no double-deletion
   - Safe cyclic dependency resolution via deferred construction
   - Thread-safe initialization via `std::call_once`

```cpp
// ❌ WRONG: Eager construction, unclear ownership
d->yourService = std::make_unique<YourService>(
    std::make_unique<YourDataProviderJsonImpl>(resources));

// ✅ CORRECT: LazyPtr owns component, dependencies injected as raw pointers
d->yourDataProvider = LazyPtr<IYourDataProvider>([this] { 
    return std::make_unique<YourDataProviderJsonImpl>(resources); 
});
d->yourService = LazyPtr<IYourService>([this] { 
    return std::make_unique<YourService>(d->yourDataProvider.get()); 
});

// Usage: component created on first .get() call
auto* service = d->yourService.get(); // thread-safe, lazy
```

## 4. Create Widget (UI Component)

```cpp
// src/Launcher/widgets/your_module/your_widget.h
#pragma once
#include <QWidget>
#include <memory>

class YourService;

class YourWidget : public QWidget {
    Q_OBJECT
public:
    explicit YourWidget(YourService* yourService, QWidget* parent = nullptr);
    ~YourWidget() override;

signals:
    void actionCompleted();

private slots:
    void onButtonClicked();

private:
    class Private;
    std::unique_ptr<Private> d;
};
```

```cpp
// src/Launcher/widgets/your_module/your_widget.cpp
#include "Launcher/mdi_child_window.h"
#include <QObject>
#include <memory>

class YourWidget::Private {
public:
	Private(YourWidget* parent) : q(parent) {}
	YourWidget* q;

	YourService* yourService = nullptr;

	void setupUI();
};

YourWidget::YourWidget(YourService* yourService, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->yourService = yourService;
}

YourWidget::~YourWidget() = default; // or implementation

void YourWidget::Private::setupUI() {
}
```

## 5. Create MDI Window

```cpp
// src/Launcher/widgets/your_module/your_window.h
#pragma once
#include "Launcher/mdi_child_window.h"
#include <QObject>
#include <memory>

class YourWindow : public MdiChildWindow {
    Q_OBJECT
public:
    explicit YourWindow(const QString& id, QWidget* parent = nullptr);
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
// src/Launcher/widgets/your_module/your_window.h
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/commands/command_context.h"

class YourWindow::Private {
public:
  Private(YourWindow* parent) : q(parent) {}
  YourWindow* q;

	YourService* yourService = nullptr;
	YourWidget* yourWidget = nullptr;
};

YourWindow::YourWindow(const QString& id, QWidget* parent)
  : MdiChildWindow(id, parent)
  , d(std::make_unique<Private>(this)) {
}

YourWindow::~YourWindow() = default; // or implementation

bool YourWindow::handleCommand(const QString& commandName,
  const QStringList& args,
  CommandContext* context) {
	if (commandName == "create") {
		auto controller = context->applicationController();
		auto services = controller->services();
		d->yourService = services->yourService();
		d->yourWidget = new YourWidget(
			d->yourService,
			this);

		setWidget(d->yourWidget);

		// add connections if needed
		// connect(d->yourWidget, &YourWidget::signal, this, &YourWindow::slot);

		return true;
	}

	return false;
}
```

## 6. Register Service in `services.cpp`

```cpp
// src/Launcher/services.h - add forward declaration
class YourService;
class IYourDataProvider;

// src/Launcher/services.h - add method
YourService* yourService() const;

// src/Launcher/services.cpp - add includes
#include "ApplicationLayer/your_module/your_service.h"
#include "DataLayer/your_module/your_data_provider_json_impl.h"
#include "DataLayer/your_module/i_your_data_provider.h"

// src/Launcher/services.cpp - add to Private class
std::unique_ptr<YourService> yourService;
std::unique_ptr<IYourDataProvider> yourDataProvider;

// src/Launcher/services.cpp - create in constructor
d->yourDataProvider = std::make_unique<YourDataProviderJsonImpl>(resources);
d->yourService = std::make_unique<YourService>(d->yourDataProvider.get());

// src/Launcher/services.cpp - add accessor
YourService* Services::yourService() const {
    return d->yourService.get();
}
```

## 7. Register Window in `windows_builder.cpp`

```cpp
#include "Launcher/widgets/your_module/your_window.h"

// In constructor:
d->factory.emplace("your-window-type", [](const QString& id, QWidget* parent) {
    return new YourWindow(id, parent);
});
```

## 8. Create Window via Command

```cpp
// Via code:
controller->executeCommandByName("window-create", QStringList{ "target:your-window-type", "id:unique-id" });

// Via console:
window-create target:your-window-type id:unique-id
```
