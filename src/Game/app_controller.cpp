#include "Game/app_controller.h"
#include "Game/mdi_child_window.h"
#include "Game/commands/command.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/commands/cmd/windows_close_all_cmd.h"
#include "Game/commands/cmd/windows_close_cmd.h"
#include "Game/commands/cmd/windows_list_cmd.h"
#include "Game/commands/cmd/windows_handle_cmd.h"
#include <QMdiArea>
#include <QPointer>
#include <QMdiSubWindow>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include <QUuid>

class ApplicationController::Private {
public:
	Private(ApplicationController* parent)
	: q(parent) {
	}

	ApplicationController* q;

	// Генерация уникального идентификатора окна
	QString generateUniqueWindowId(const QString& baseName = "window") const {
		QString candidate;
		do {
			candidate = QString("%1_%2")
				.arg(baseName)
				.arg(++windowIdCounter);
		} while (windowRegistry.contains(candidate));

		return candidate;
	}

	std::unique_ptr<CommandProcessor> commandProcessor;
	std::unique_ptr<CommandContext> commandContext;
	QPointer<QMdiArea> mdiArea;

	QHash<QString, QPointer<MdiChildWindow>> windowRegistry;
	QHash<MdiChildWindow*, QString> windowToId;
	QPointer<MdiChildWindow> activeWindow;

	// Счётчик для генерации уникальных ID
	mutable quint64 windowIdCounter = 0;
};

ApplicationController::ApplicationController(QObject* parent)
: QObject(parent)
,	d(new Private(this)) {

	// Создание процессора команд
	d->commandProcessor = std::make_unique<CommandProcessor>();

	// Создание контекста с обратной ссылкой на этот контроллер
	d->commandContext = std::make_unique<CommandContext>(this);

	// Регистрация встроенных системных команд
	d->commandProcessor->registerCommand(std::make_unique<ListWindowsCommand>());
	d->commandProcessor->registerCommand(std::make_unique<CloseWindowCommand>());
	d->commandProcessor->registerCommand(std::make_unique<CloseAllWindowsCommand>());
	d->commandProcessor->registerCommand(std::make_unique<HandleWindowsCommand>());

	qInfo() << "ApplicationController initialized with"
		<< d->commandProcessor->availableCommands().size()
		<< "available commands";
}

ApplicationController::~ApplicationController() {
	qInfo() << "ApplicationController destroyed";
}


CommandProcessor* ApplicationController::commandProcessor() const {
	return d->commandProcessor.get();
}

CommandContext* ApplicationController::commandContext() const {
	return d->commandContext.get();
}

void ApplicationController::setMdiArea(QMdiArea* mdiArea) {
	if (d->mdiArea == mdiArea) {
		return;
	}

	// Отключение от предыдущей MDI области
	if (d->mdiArea) {
		disconnect(d->mdiArea, &QMdiArea::subWindowActivated, 
			this, &ApplicationController::onSubWindowActivated);
	}

	d->mdiArea = mdiArea;

	// Подключение к новой MDI области
	if (d->mdiArea) {
		connect(d->mdiArea, &QMdiArea::subWindowActivated,
			this, &ApplicationController::onSubWindowActivated);

		// Регистрация уже существующих окон
		for (QMdiSubWindow* subWindow : d->mdiArea->subWindowList()) {
			if (auto* window = qobject_cast<MdiChildWindow*>(subWindow->widget())) {
				registerWindow(window);
			}
		}
	}
}

QMdiArea* ApplicationController::mdiArea() const {
	return d->mdiArea.data();
}

QString ApplicationController::registerWindow(MdiChildWindow* window, const QString& requestedId) {
	if (!window) {
		qWarning() << "Attempted to register null window";
		return QString();
	}

	// Проверка: окно уже зарегистрировано?
	if (d->windowToId.contains(window)) {
		QString existingId = d->windowToId.value(window);
		if (requestedId.isEmpty() || requestedId == existingId) {
			return existingId; // уже зарегистрировано с тем же или безымянным ID
		}
		// Если запрошен другой ID — сначала удаляем старую регистрацию
		unregisterWindow(window);
	}

	// Генерация или валидация идентификатора
	QString windowId = requestedId;
	if (windowId.isEmpty()) {
		//windowId = d->generateUniqueWindowId(window->windowType());
		windowId = d->generateUniqueWindowId();
	}
	else {
		// Проверка уникальности
		int suffix = 1;
		QString baseId = windowId;
		while (d->windowRegistry.contains(windowId)) {
			windowId = QString("%1_%2").arg(baseId).arg(suffix++);
		}
	}

	// Регистрация окна
	d->windowRegistry[windowId] = window;
	d->windowToId[window] = windowId;

	// Отслеживание уничтожения окна
	connect(window, &QObject::destroyed,
		this, &ApplicationController::onSubWindowDestroyed);

	qInfo() << "Window registered:" << windowId << "type:" << window->windowType();

	emit windowCreated(window, windowId);
	return windowId;
}

void ApplicationController::unregisterWindow(MdiChildWindow* window) {
	if (!window || !d->windowToId.contains(window)) return;

	QString windowId = d->windowToId.value(window);

	// Очистка реестров
	d->windowRegistry.remove(windowId);
	d->windowToId.remove(window);

	// Отключение сигнала (на всякий случай)
	disconnect(window, &QObject::destroyed,
		this, &ApplicationController::onSubWindowDestroyed);

	qInfo() << "Window unregistered:" << windowId;

	emit windowDestroyed(windowId);
}

MdiChildWindow* ApplicationController::findWindowById(const QString& windowId) const {
	if (auto ptr = d->windowRegistry.value(windowId)) {
		return ptr.data();
	}
	return nullptr;
}

QList<MdiChildWindow*> ApplicationController::allWindows() const {
	QList<MdiChildWindow*> result;
	for (auto it = d->windowRegistry.constBegin(); it != d->windowRegistry.constEnd(); ++it) {
		if (auto window = it.value().data()) {
			result.append(window);
		}
	}
	return result;
}

MdiChildWindow* ApplicationController::activeWindow() const {
	return d->activeWindow.data();
}

bool ApplicationController::executeCommand(const QString& commandText, QObject* requester) {
	Q_UNUSED(requester); // Может использоваться для аудита/логгирования

	if (!d->commandProcessor || commandText.trimmed().isEmpty()) {
		return false;
	}

	QElapsedTimer timer;
	timer.start();

	bool success = d->commandProcessor->execute(commandText, d->commandContext.get());

	qint64 elapsed = timer.elapsed();
	QString cmdName = commandText.split(' ', Qt::SkipEmptyParts).value(0);

	if (success) {
		emit commandExecuted(cmdName, elapsed);
	}
	else {
		emit commandFailed(cmdName, "Execution failed or command not found");
	}

	return success;
}

bool ApplicationController::executeCommandByName(const QString& commandName,
	const QStringList& args,
	QObject* requester) {
	Q_UNUSED(requester);

	if (!d->commandProcessor) {
		return false;
	}

	auto* command = d->commandProcessor->findCommand(commandName);
	if (!command) {
		emit commandFailed(commandName, "Command not found");
		return false;
	}

	QElapsedTimer timer;
	timer.start();

	bool success = false;
	try {
		success = command->execute(d->commandContext.get(), args);
	}
	catch (const std::exception& e) {
		emit commandFailed(commandName, QString::fromUtf8(e.what()));
		return false;
	}

	qint64 elapsed = timer.elapsed();
	if (success) {
		emit commandExecuted(commandName, elapsed);
	}
	else {
		emit commandFailed(commandName, "Command execution returned false");
	}

	return success;
}

void ApplicationController::onSubWindowActivated(QMdiSubWindow* subWindow) {
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

void ApplicationController::onSubWindowDestroyed(QObject* obj) {
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

QList<QPair<QPointer<MdiChildWindow>, QString>> ApplicationController::windowEntries() const {
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

QPair<QPointer<MdiChildWindow>, QString> ApplicationController::activeWindowEntry() const {
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

bool ApplicationController::closeWindowById(const QString& windowId) {
	if (auto window = findWindowById(windowId)) {
		window->close();
		return true;
	}
	return false;
}

int ApplicationController::closeAllWindows() {
	auto entries = windowEntries();
	for (const auto& entry : entries) {
		if (entry.first) {
			entry.first->close();
		}
	}
	return entries.size();
}
