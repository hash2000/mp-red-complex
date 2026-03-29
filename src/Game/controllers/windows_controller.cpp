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
	QPointer<MdiArea> mdiArea;
	QHash<QString, QPointer<MdiChildWindow>> windowRegistry;
	QHash<MdiChildWindow*, QString> windowToId;
	QPointer<MdiChildWindow> activeWindow;
};

WindowsController::WindowsController()
: d(std::make_unique<Private>(this)) {

}

WindowsController::~WindowsController() = default;


void WindowsController::setMdiArea(MdiArea* mdiArea) {
	if (d->mdiArea == mdiArea) {
		return;
	}

	// Отключение от предыдущей MDI области
	if (d->mdiArea) {
		disconnect(d->mdiArea, &MdiArea::subWindowActivated,
			this, &WindowsController::onSubWindowActivated);
	}

	d->mdiArea = mdiArea;

	// Подключение к новой MDI области
	if (d->mdiArea) {
		connect(d->mdiArea, &MdiArea::subWindowActivated,
			this, &WindowsController::onSubWindowActivated);

		// Регистрация уже существующих окон
		for (QMdiSubWindow* subWindow : d->mdiArea->subWindowList()) {
			if (auto window = qobject_cast<MdiChildWindow*>(subWindow->widget())) {
				registerWindow(window);
			}
		}
	}
}

MdiArea* WindowsController::mdiArea() const {
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
	auto connection = connect(window, &QObject::destroyed,
		this, &WindowsController::onSubWindowDestroyed);
	
	qInfo() << "Window registered:" << windowId << "type:" << window->windowType();
	qInfo() << "  window pointer:" << window;
	qInfo() << "  window parent:" << window->parent();
	qInfo() << "  connection:" << (connection ? "connected" : "failed");
	
	// Если это QMdiSubWindow, проверим его widget
	if (auto subWindow = qobject_cast<QMdiSubWindow*>(window)) {
		qInfo() << "  subWindow widget:" << subWindow->widget();
	}

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
	qDebug() << "Window destroyed, obj type:" << obj->metaObject()->className();
	qDebug() << "obj pointer:" << obj;
	
	// Ищем окно в реестре по указателю (не по типу!)
	// Во время уничтожения qobject_cast может не работать, так как объект уже частично разрушен
	MdiChildWindow* windowToUnregister = nullptr;
	
	// Проверяем, является ли obj зарегистрированным окном
	for (auto it = d->windowToId.constBegin(); it != d->windowToId.constEnd(); ++it) {
		if (it.key() == obj) {
			qDebug() << "Found window by pointer match:" << it.key();
			windowToUnregister = it.key();
			break;
		}
	}
	
	// Если не нашли по точному совпадению, проверяем через qobject_cast
	if (!windowToUnregister) {
		if (auto window = qobject_cast<MdiChildWindow*>(obj)) {
			qDebug() << "Cast to MdiChildWindow succeeded:" << window;
			windowToUnregister = window;
		}
	}
	
	// Если не нашли, проверяем, является ли obj виджетом внутри QMdiSubWindow
	if (!windowToUnregister) {
		for (auto it = d->windowToId.constBegin(); it != d->windowToId.constEnd(); ++it) {
			auto window = it.key();
			if (auto subWindow = qobject_cast<QMdiSubWindow*>(window)) {
				if (subWindow->widget() == obj) {
					qDebug() << "Found window by widget():" << window;
					windowToUnregister = window;
					break;
				}
			}
		}
	}
	
	if (windowToUnregister) {
		unregisterWindow(windowToUnregister);
	}
	else {
		qDebug() << "Window not found in registry, obj:" << obj;
		// Выведем все окна в реестре для отладки
		for (auto it = d->windowToId.constBegin(); it != d->windowToId.constEnd(); ++it) {
			auto window = it.key();
			qDebug() << "Registered window:" << window << "id:" << it.value() 
					 << "parent:" << window->parent() 
					 << "parent type:" << (window->parent() ? window->parent()->metaObject()->className() : "null");
		}
	}
}

QList<QPair<QPointer<MdiChildWindow>, QString>> WindowsController::windowEntries() const {
	QList<QPair<QPointer<MdiChildWindow>, QString>> result;
	result.reserve(d->windowToId.size());

	for (auto it = d->windowToId.constBegin(); it != d->windowToId.constEnd(); ++it) {
		MdiChildWindow* rawPtr = it.key();
		const QString& windowId = it.value();

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
