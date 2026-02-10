#include "Game/controllers/windows_controller.h"
#include "Game/mdi_child_window.h"
#include <QPointer>
#include <QMdiSubWindow>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include <QUuid>

class WindowsController::Private {
public:
	Private(WindowsController* parent)
	:	q(parent) {
	}

	WindowsController* q;
	QPointer<QMdiArea> mdiArea;
	QHash<QString, QPointer<MdiChildWindow>> windowRegistry;
	QHash<MdiChildWindow*, QString> windowToId;
	QPointer<MdiChildWindow> activeWindow;
};

WindowsController::WindowsController()
: d(std::make_unique<Private>(this)) {

}

WindowsController::~WindowsController() = default;


void WindowsController::setMdiArea(QMdiArea* mdiArea) {
	if (d->mdiArea == mdiArea) {
		return;
	}

	// Отключение от предыдущей MDI области
	if (d->mdiArea) {
		disconnect(d->mdiArea, &QMdiArea::subWindowActivated,
			this, &WindowsController::onSubWindowActivated);
	}

	d->mdiArea = mdiArea;

	// Подключение к новой MDI области
	if (d->mdiArea) {
		connect(d->mdiArea, &QMdiArea::subWindowActivated,
			this, &WindowsController::onSubWindowActivated);

		// Регистрация уже существующих окон
		for (QMdiSubWindow* subWindow : d->mdiArea->subWindowList()) {
			if (auto* window = qobject_cast<MdiChildWindow*>(subWindow->widget())) {
				registerWindow(window);
			}
		}
	}
}

QMdiArea* WindowsController::mdiArea() const {
	return d->mdiArea.data();
}

bool WindowsController::registerWindow(MdiChildWindow* window) {
	if (!window) {
		qWarning() << "Attempted to register null window";
		return false;
	}

	// Генерация или валидация идентификатора
	QString windowId = window->windowId();
	if (d->windowRegistry.contains(windowId)) {
		qWarning() << "Window allready registered" << windowId;
		return false;
	}

	// Регистрация окна
	d->windowRegistry[windowId] = window;
	d->windowToId[window] = windowId;

	// Отслеживание уничтожения окна
	connect(window, &QObject::destroyed,
		this, &WindowsController::onSubWindowDestroyed);

	qInfo() << "Window registered:" << windowId << "type:" << window->windowType();

	emit windowCreated(window, windowId);
	return true;
}

void WindowsController::unregisterWindow(MdiChildWindow* window) {
	if (!window || !d->windowToId.contains(window)) {
		return;
	}

	QString windowId = d->windowToId.value(window);

	// Очистка реестров
	d->windowRegistry.remove(windowId);
	d->windowToId.remove(window);

	// Отключение сигнала (на всякий случай)
	disconnect(window, &QObject::destroyed,
		this, &WindowsController::onSubWindowDestroyed);

	qInfo() << "Window unregistered:" << windowId;

	emit windowDestroyed(windowId);
}

MdiChildWindow* WindowsController::findWindowById(const QString& windowId) const {
	if (auto ptr = d->windowRegistry.value(windowId)) {
		return ptr.data();
	}
	return nullptr;
}

QList<MdiChildWindow*> WindowsController::allWindows() const {
	QList<MdiChildWindow*> result;
	for (auto it = d->windowRegistry.constBegin(); it != d->windowRegistry.constEnd(); ++it) {
		if (auto window = it.value().data()) {
			result.append(window);
		}
	}
	return result;
}

MdiChildWindow* WindowsController::activeWindow() const {
	return d->activeWindow.data();
}


void WindowsController::onSubWindowActivated(QMdiSubWindow* subWindow) {
	MdiChildWindow* newWindow = nullptr;
	if (subWindow && subWindow->widget()) {
		newWindow = qobject_cast<MdiChildWindow*>(subWindow->widget());
	}

	MdiChildWindow* oldWindow = d->activeWindow.data();

	if (newWindow != oldWindow) {
		d->activeWindow = newWindow;
		emit activeWindowChanged(newWindow, oldWindow);
	}
}

void WindowsController::onSubWindowDestroyed(QObject* obj) {
	// obj может быть как MdiChildWindow, так и QMdiSubWindow
	if (auto* window = qobject_cast<MdiChildWindow*>(obj)) {
		unregisterWindow(window);
	}
	else if (auto* subWindow = qobject_cast<QMdiSubWindow*>(obj)) {
		// Если уничтожено само подокно — ищем widget внутри реестра
		for (auto it = d->windowToId.constBegin(); it != d->windowToId.constEnd(); ++it) {
			if (it.key()->parent() == subWindow) {
				unregisterWindow(it.key());
				break;
			}
		}
	}
}

QList<QPair<QPointer<MdiChildWindow>, QString>> WindowsController::windowEntries() const {
	QList<QPair<QPointer<MdiChildWindow>, QString>> result;
	result.reserve(d->windowToId.size());

	for (auto it = d->windowToId.constBegin(); it != d->windowToId.constEnd(); ++it) {
		MdiChildWindow* rawPtr = it.key();
		const QString& windowId = it.value();

		// 🔑 КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: проверка сырого указателя на nullptr
		if (rawPtr == nullptr) {
			continue;
		}

		// Дополнительная защита: проверяем, что окно всё ещё в основном реестре
		// (защита от рассинхронизации реестров при ошибках в логике удаления)
		auto regIt = d->windowRegistry.constFind(windowId);
		if (regIt == d->windowRegistry.constEnd() || regIt.value().isNull()) {
			continue; // окно было удалено, но не убрано из обратного реестра
		}

		// Оборачиваем в QPointer для безопасности вызывающего кода
		result.append({ rawPtr, windowId });
	}

	return result;
}

QPair<QPointer<MdiChildWindow>, QString> WindowsController::activeWindowEntry() const {
	if (d->activeWindow.isNull()) {
		return { nullptr, QString() };
	}

	MdiChildWindow* activeRaw = d->activeWindow.data();
	if (!activeRaw) {
		return { nullptr, QString() };
	}

	// Поиск ID через обратный реестр (безопасно, так как окно зарегистрировано)
	auto it = d->windowToId.constFind(activeRaw);
	QString id = (it != d->windowToId.constEnd()) ? it.value() : QString();

	return { d->activeWindow, id };
}

bool WindowsController::closeWindowById(const QString& windowId) {
	if (auto window = findWindowById(windowId)) {
		window->close();
		return true;
	}
	return false;
}

int WindowsController::closeAllWindows() {
	auto entries = windowEntries();
	for (const auto& entry : entries) {
		if (entry.first) {
			entry.first->close();
		}
	}
	return entries.size();
}
