#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/i_command.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/system_options/system_options_registry.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/console/console_json.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/doc_tab_panel/document_tabs_order.h"
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
	/// Корневой контекст системных настроек `.options`.
	std::shared_ptr<CommandContext> optionsContext;
	std::unique_ptr<SystemOptionsRegistry> systemOptions;
	std::unique_ptr<WidgetRegistry> widgetRegistry;
	QPointer<MainWindow> mainWindow;
	Resources* resources;

	/// Фоновый поток периодической очистки мёртвых контекстов.
	QThread* cleanupThread = nullptr;

	std::shared_ptr<CommandContext> createLocalContext();
	int cleanupIntervalMs() const;

	/// Встроенные системные параметры приложения.
	void registerSystemOptions();
};

CommandController::CommandController(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>()) {

	d->resources = resources;
	d->globalContext = std::make_shared<CommandContext>(this, std::shared_ptr<CommandContext>() /*is global context*/, "");
	d->widgetRegistry = std::make_unique<WidgetRegistry>();
	d->systemOptions = std::make_unique<SystemOptionsRegistry>();
	d->registerSystemOptions();
}

CommandController::~CommandController() {
	stopContextCleanup();
	d->documentsContext.reset();
	d->optionsContext.reset();
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

void CommandController::Private::registerSystemOptions() {
	if (!systemOptions) {
		return;
	}

	// Правило сортировки/группировки вкладок документов: допустимые значения
	// берутся из реестра правил, поэтому новое правило достаточно добавить
	// в DocumentTabsOrderRegistry — настройка и UI настроек обновятся сами.
	registerDocumentTabsOrderOption(*systemOptions);

	// Глобальные параметры текстовых документов: из них документы берут
	// значения по умолчанию при создании (редактор кода и виртуальный
	// редактор больших файлов).
	auto registerTextOption = [this](const char* name, const QString& title,
		const QString& description, SystemOptionType type, const QVariant& value) {
		SystemOption option;
		option.name = QString::fromLatin1(name);
		option.title = title;
		option.description = description;
		option.type = type;
		option.defaultValue = value;
		systemOptions->registerOption(option);
	};

	registerTextOption(SystemOptions::kDocumentsTextBufferLimit,
		QStringLiteral("Предел буфера текста, МБ"),
		QStringLiteral("Сколько памяти отводится под изменённые страницы "
			"виртуального редактора больших файлов. При превышении страницы "
			"выгружаются во временный файл. 0 — без предела."),
		SystemOptionType::Integer, SystemOptions::kDocumentsTextBufferLimitDefaultMb);
	registerTextOption(SystemOptions::kDocumentsTextWordWrap,
		QStringLiteral("Перенос строк текста"),
		QStringLiteral("Переносить длинные строки в редакторе кода и в "
			"виртуальном редакторе больших файлов."),
		SystemOptionType::Boolean, false);
	registerTextOption(SystemOptions::kDocumentsTextFontSize,
		QStringLiteral("Размер шрифта текста"),
		QStringLiteral("Размер шрифта текстовых документов в пунктах. "
			"0 — системный размер."),
		SystemOptionType::Integer, 0);
	registerTextOption(SystemOptions::kDocumentsTextFontFamily,
		QStringLiteral("Шрифт текста"),
		QStringLiteral("Семейство шрифта текстовых документов. "
			"Пустая строка — системный моноширинный шрифт."),
		SystemOptionType::Text, QString());
	registerTextOption(SystemOptions::kDocumentsTextEncoding,
		QStringLiteral("Кодировка текста"),
		QStringLiteral("Кодировка чтения и записи текстовых документов "
			"(имя кодека QTextCodec, например UTF-8 или Windows-1251)."),
		SystemOptionType::Text,
		QString::fromLatin1(SystemOptions::kDocumentsTextEncodingDefault));
}

void CommandController::init() {
	initSystemContexts();

	initContext();

	// Фоновый поток периодически убирает контексты, владельцы которых
	// уже уничтожены.
	startContextCleanup();

	const auto controller = d->globalContext->commandController();
	controller->printSuccess(QString("CommandController initialized with %1 available commands")
		.arg(commandProcessor()->availableCommands().size()));
}

void CommandController::initSystemContexts() {
	if (!d->globalContext) {
		return;
	}

	// Системные параметры — переменные глобального контекста. Документы
	// не должны менять их сами по себе, поэтому контекст `.documents`
	// открывается только для чтения, а документы работают в режиме Lazy:
	// запись создаёт их собственные переменные.
	d->documentsContext = d->globalContext->documentsContext();
	d->documentsContext->setVariablesMode(CommandContext::VariablesMode::ReadOnly);

	// Привилегированный контекст системных настроек: документ с ролью
	// DocumentRule::SystemOptions работает в нём в режиме Mutable.
	d->optionsContext = d->globalContext->optionsContext();

	// Значения по умолчанию записываются в глобальный контекст, поэтому
	// настройки существуют независимо от того, открыт ли их документ.
	if (d->systemOptions) {
		const int applied = d->systemOptions->applyDefaults(*d->globalContext);
		if (applied > 0) {
			qDebug() << "CommandController: applied" << applied << "system option defaults";
		}
	}
}

WidgetRegistry* CommandController::widgets() const {
	return d->widgetRegistry.get();
}

SystemOptionsRegistry* CommandController::systemOptions() const {
	return d->systemOptions.get();
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
