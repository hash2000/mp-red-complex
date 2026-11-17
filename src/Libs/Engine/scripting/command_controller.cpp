#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/i_command.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/console/console_json.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Resources/resources.h"

#include <QDebug>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <QUuid>

namespace {
/// Период фоновой очистки мёртвых контекстов по умолчанию.
constexpr int kDefaultCleanupIntervalMs = 30000;
}

class CommandController::Private {
public:
	std::shared_ptr<CommandContext> globalContext;
	/// Корневой контекст документов `.documents`; владеется контроллером,
	/// иначе он уничтожался бы вместе с локальной ссылкой.
	std::shared_ptr<CommandContext> documentsContext;
	std::unique_ptr<WidgetRegistry> widgetRegistry;
	QPointer<MainWindow> mainWindow;
	Resources* resources;

	/// Фоновый поток периодической очистки мёртвых контекстов.
	QThread* cleanupThread = nullptr;

	std::shared_ptr<CommandContext> createLocalContext();
	int cleanupIntervalMs() const;
};

CommandController::CommandController(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>()) {

	d->resources = resources;
	d->globalContext = std::make_shared<CommandContext>(this, std::shared_ptr<CommandContext>() /*is global context*/, "");
	d->widgetRegistry = std::make_unique<WidgetRegistry>();
}

CommandController::~CommandController() {
	stopContextCleanup();
	d->globalContext.reset();
}

int CommandController::Private::cleanupIntervalMs() const {
	int interval = kDefaultCleanupIntervalMs;
	if (resources) {
		interval = resources->Variables.get(
			"command.context.cleanupIntervalMs", interval).toInt();
	}

	return interval > 0 ? interval : kDefaultCleanupIntervalMs;
}

void CommandController::startContextCleanup() {
	if (d->cleanupThread) {
		return;
	}

	d->cleanupThread = new QThread(this);
	d->cleanupThread->setObjectName(QStringLiteral("CommandContextCleanup"));

	// Таймер живёт в фоновом потоке, а очистка выполняется в потоке
	// контроллера: реестр контекстов принадлежит GUI-потоку.
	auto* timer = new QTimer();
	timer->setInterval(d->cleanupIntervalMs());
	timer->moveToThread(d->cleanupThread);
	qDebug() << "CommandController: context cleanup interval" << timer->interval() << "ms";

	QObject::connect(d->cleanupThread, &QThread::started, timer, [timer]() {
		timer->start();
	});
	QObject::connect(timer, &QTimer::timeout, this, [this]() {
		pruneDeadContexts();
	}, Qt::QueuedConnection);
	QObject::connect(d->cleanupThread, &QThread::finished, timer, &QObject::deleteLater);

	d->cleanupThread->start();
}

void CommandController::stopContextCleanup() {
	if (!d->cleanupThread) {
		return;
	}

	d->cleanupThread->quit();
	if (!d->cleanupThread->wait(2000)) {
		qWarning() << "CommandController: context cleanup thread did not stop in time";
	}

	d->cleanupThread = nullptr;
}

int CommandController::pruneDeadContexts() {
	if (!d->globalContext) {
		return 0;
	}

	return d->globalContext->pruneDeadContexts();
}

void CommandController::init() {
	// Корневой контекст документов: в нём живут документы-синглтоны,
	// дочерние документы создаются в `.documents.{documentId}`.
	d->documentsContext = d->globalContext->documentsContext();

	initContext();

	// Фоновый поток периодически убирает контексты, владельцы которых
	// уже уничтожены.
	startContextCleanup();

	const auto controller = d->globalContext->commandController();
	controller->printSuccess(QString("CommandController initialized with %1 available commands")
		.arg(commandProcessor()->availableCommands().size()));
}

WidgetRegistry* CommandController::widgets() const {
	return d->widgetRegistry.get();
}

MainWindow* CommandController::mainWindow() const {
	return d->mainWindow.data();
}

void CommandController::setMainWindow(MainWindow* window) {
	d->mainWindow = window;
}

Resources* CommandController::resources() const {
	return d->resources;
}

std::shared_ptr<CommandContext> CommandController::globalContext() {
	return d->globalContext;
}

std::shared_ptr<CommandContext> CommandController::documentsContext() {
	if (!d->globalContext) {
		return nullptr;
	}

	if (!d->documentsContext) {
		d->documentsContext = d->globalContext->documentsContext();
	}

	return d->documentsContext;
}

bool CommandController::execute(const QString& commandText, QObject* requester) {
	return commandProcessor()->execute(commandText, d->createLocalContext());
}

bool CommandController::executeCommand(const QString& commandName, const QMap<QString, QString>& args, QObject* requester) {
	return commandProcessor()->executeCommand(commandName, args, d->createLocalContext());
}

std::shared_ptr<CommandContext> CommandController::Private::createLocalContext() {
	auto nsLocal = QUuid::createUuid().toString(QUuid::WithoutBraces)
		.remove('-');
	return globalContext->createScopedContext(nsLocal);
}

void CommandController::print(const QString& message, const QString& styleClass, const QString& type) {
	QMetaObject::invokeMethod(this, [this, message, styleClass, type]() {
		emit outputRequested(message, styleClass, type);
	}, Qt::QueuedConnection);
}

void CommandController::print(const ConsoleTable& table, const QString& styleClass) {
	const auto html = table.toHtml();
	if (html.isEmpty()) {
		return;
	}

	print(html, styleClass, kCommandPrintStyle_Table);
}

void CommandController::print(const ConsoleJson& json, const QString& styleClass) {
	const auto html = json.toHtml();
	if (html.isEmpty()) {
		return;
	}

	print(html, styleClass, kCommandPrintStyle_Json);
}

void CommandController::printSystem(const QString& message) {
	print(message, "system");
}

void CommandController::printError(const QString& message) {
	print(message, "error");
}

void CommandController::printSuccess(const QString& message) {
	print(message, "success");
}

void CommandController::printWarning(const QString& message) {
	print(message, "warning");
}
