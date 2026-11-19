#include "Libs/Engine/commands/widget_zone_command.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <QThread>
#include <QUuid>

#include <exception>

namespace {

constexpr const char* kActionList = "list";
constexpr const char* kActionCreate = "create";
constexpr const char* kActionClose = "close";
constexpr const char* kActionShow = "show";

} // namespace

WidgetZoneCommand::WidgetZoneCommand(const QString& name, MainWindow::WidgetKind kind, QObject* parent)
	: ICommand(parent)
	, _name(name)
	, _kind(kind) {
}

QString WidgetZoneCommand::name() const {
	return _name;
}

QString WidgetZoneCommand::description() const {
	return QString("Управление областью интерфейса: %1").arg(_name);
}

QString WidgetZoneCommand::help() const {
	return QString(
		"%1 action=create, target=<xtype или класс>[, id=<id>][, name=<name>]"
		"[, title=<title>][, glyph=<glyph>][, tooltip=<tooltip>][, <параметры виджета>...]\n"
		"%1 action=show, id=<id>\n"
		"%1 action=close, id=<id>\n"
		"%1 action=list\n"
		"Без id генерируется UUID. Повторный id в этой же области активирует виджет,"
		" в другой области — ошибка. Для типа-синглтона id всегда равен его xtype,"
		" поэтому повторное создание активирует уже открытый документ.")
		.arg(_name);
}

bool WidgetZoneCommand::execute(std::shared_ptr<CommandContext> context) {
	if (!context || !context->commandController()) {
		return false;
	}

	auto* controller = context->commandController();
	auto* window = controller->mainWindow();
	if (!window) {
		controller->printError(QStringLiteral("Главное окно недоступно"));
		return false;
	}

	if (window->thread() != QThread::currentThread()) {
		controller->printError(QStringLiteral("Команды интерфейса выполняются только в GUI-потоке"));
		return false;
	}

	const auto action = context->variable("action").toString();
	if (action == kActionList) {
		return executeList(context);
	}
	if (action == kActionCreate) {
		return executeCreate(context);
	}
	if (action == kActionClose) {
		return executeClose(context);
	}
	if (action == kActionShow) {
		return executeShow(context);
	}

	controller->printError(QString("Использование:\n%1").arg(help()));
	return false;
}

bool WidgetZoneCommand::executeCreate(const std::shared_ptr<CommandContext>& context) {
	auto* controller = context->commandController();
	auto* window = controller->mainWindow();
	const auto target = context->variable("target").toString();
	if (target.isEmpty()) {
		controller->printError(QString("%1: не задан target").arg(_name));
		return false;
	}

	auto* registry = controller->widgets();
	if (!registry) {
		controller->printError(QStringLiteral("Реестр виджетов недоступен"));
		return false;
	}

	// Описание типа: значения по умолчанию и признак singletone.
	const auto typeInfo = registry->typeInfo(target);

	auto id = context->variable("id").toString();
	if (id.isEmpty()) {
		id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	}
	// Для singletone имя всегда равно xtype: повторный вызов активирует
	// уже открытый документ, даже если в команде передан другой id.
	id = registry->documentId(target, id);

	const auto existingKind = window->widgetKind(id);
	if (existingKind && *existingKind != _kind) {
		controller->printError(QString("%1: id занят другой областью: %2").arg(_name, id));
		return false;
	}

	const auto name = context->variable("name", id).toString();
	auto title = context->variable("title").toString();
	auto glyph = context->variable("glyph").toString();
	if (typeInfo) {
		if (title.isEmpty()) {
			title = typeInfo->title;
		}
		if (glyph.isEmpty()) {
			glyph = typeInfo->glyph;
		}
	}
	if (title.isEmpty()) {
		title = target;
	}
	const auto tooltip = context->variable("tooltip").toString();

	// Остальные параметры команды становятся начальными переменными
	// контекста документа (например, path=...).
	QVariantMap documentVariables;
	const auto commandVariables = context->ownVariables();
	const QStringList reserved = { "action", "target", "id", "name", "title", "glyph", "tooltip" };
	for (auto it = commandVariables.cbegin(); it != commandVariables.cend(); it++) {
		if (!reserved.contains(it.key())) {
			documentVariables.insert(it.key(), it.value());
		}
	}

	WidgetBuildHandler handler;
	handler.className = typeInfo ? typeInfo->className : target;
	handler.xtype = typeInfo ? typeInfo->xtype : target;
	handler.name = name;
	handler.title = title;
	handler.glyph = glyph;
	handler.singletone = typeInfo ? typeInfo->singletone : false;
	handler.parent = window;
	// Параметры запуска (action/target/...) виджет получает из контекста команды.
	handler.context = context;

	// Новый документ всегда создаётся в контексте `.documents.{id}`:
	// если команду выполнил документ (например, file explorer), родителем
	// становится его контекст, иначе — глобальный контекст приложения,
	// независимо от выбранного в консоли пространства имён.
	auto creator = context->owningDocumentContext();
	if (!creator) {
		creator = context->globalContext();
	}

	auto* widget = window->createWidget(id, _kind, title, glyph,
		[registry, target, handler]() { return registry->build(target, handler); },
		tooltip, handler.xtype, creator, documentVariables);
	if (!widget) {
		controller->printError(QString("%1: не удалось создать %2").arg(_name, target));
		return false;
	}

	controller->printSuccess(QString("%1: %2 [%3]")
		.arg(_name, existingKind ? QStringLiteral("активирован") : QStringLiteral("создан"), id));
	return true;
}

bool WidgetZoneCommand::executeClose(const std::shared_ptr<CommandContext>& context) {
	auto* controller = context->commandController();
	auto* window = controller->mainWindow();
	const auto id = context->variable("id").toString();
	if (id.isEmpty() || window->widgetKind(id) != _kind) {
		controller->printError(QString("%1: id не найден: %2").arg(_name, id));
		return false;
	}
	if (!window->closeWidget(id)) {
		controller->printError(QString("%1: не удалось закрыть: %2").arg(_name, id));
		return false;
	}
	controller->printSuccess(QString("%1: закрыт %2").arg(_name, id));
	return true;
}

bool WidgetZoneCommand::executeShow(const std::shared_ptr<CommandContext>& context) {
	auto* controller = context->commandController();
	auto* window = controller->mainWindow();
	const auto id = context->variable("id").toString();
	if (id.isEmpty() || window->widgetKind(id) != _kind || !window->showWidget(id)) {
		controller->printError(QString("%1: id не найден: %2").arg(_name, id));
		return false;
	}
	controller->printSuccess(QString("%1: активирован %2").arg(_name, id));
	return true;
}

bool WidgetZoneCommand::executeList(const std::shared_ptr<CommandContext>& context) {
	auto* controller = context->commandController();
	auto* window = controller->mainWindow();

	ConsoleTable table({ "Id", "Title", "Type" });
	for (const auto& id : window->widgetIds(_kind)) {
		auto* widget = window->widget(id);
		if (!widget) {
			continue;
		}
		table.addRow(QList<QVariant>{ id, window->widgetTitle(id), window->widgetType(id) });
	}

	if (table.isEmpty()) {
		controller->print(QString("%1: пусто").arg(_name));
	}
	else {
		controller->print(table);
	}
	return true;
}
