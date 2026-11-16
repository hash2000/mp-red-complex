#include "Libs/Engine/scripting/application/class_manager.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Resources/resources.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QVariant>

#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

namespace {

QString nodeString(const YAML::Node& node, const char* key) {
	if (!node || !node[key] || !node[key].IsScalar()) {
		return QString();
	}

	return QString::fromStdString(node[key].as<std::string>());
}

/// Необязательный булев ключ: отсутствие ключа — std::nullopt,
/// ошибка преобразования — предупреждение и std::nullopt.
std::optional<bool> nodeBool(const YAML::Node& node, const char* key) {
	if (!node || !node[key] || !node[key].IsScalar()) {
		return std::nullopt;
	}

	try {
		return node[key].as<bool>();
	}
	catch (const YAML::Exception& e) {
		qWarning() << "ClassManager: bad boolean value for" << key << ":" << e.what();
		return std::nullopt;
	}
}

QStringList nodeStringList(const YAML::Node& node, const char* key) {
	QStringList result;

	if (!node || !node[key] || !node[key].IsSequence()) {
		return result;
	}

	for (const auto& item : node[key]) {
		if (item.IsScalar()) {
			result << QString::fromStdString(item.as<std::string>());
		}
	}

	return result;
}

/// Идентичность элемента последовательности при слиянии:
/// ключ одноэлементной мапы, иначе name, иначе xtype.
std::optional<std::string> sequenceIdentity(const YAML::Node& node) {
	if (!node || !node.IsMap()) {
		return std::nullopt;
	}

	if (node.size() == 1) {
		return node.begin()->first.Scalar();
	}

	if (node["name"] && node["name"].IsScalar()) {
		return std::string("name:") + node["name"].Scalar();
	}

	if (node["xtype"] && node["xtype"].IsScalar()) {
		return std::string("xtype:") + node["xtype"].Scalar();
	}

	return std::nullopt;
}

QString classFilePath(const QString& root, const QString& className) {
	QString relative = className;
	relative.replace('.', '/');
	return root + QStringLiteral("/") + relative + QStringLiteral(".yaml");
}

} // namespace

class ClassManager::Private {
public:
	struct Declaration {
		QString name;
		QString xtype;
		QString extends;
		QString title;
		QStringList requiredClasses;
		QStringList visibleClasses;
		YAML::Node content;
		bool resolved = false;
		bool resolving = false;
	};

	Private(ClassManager* parent, Resources* resources, CommandController* commandController)
		: q(parent)
		, resources(resources)
		, commandController(commandController) {
	}

	QWidget* createViewport(const BootstrapConfig::Application& application);

private:
	Declaration* getDeclaration(const QString& name);
	Declaration* findDeclarationByType(const QString& type);
	Declaration* findDeclarationForType(const Declaration& scope, const QString& type);
	QWidget* buildViewport(Declaration& declaration);
	void applyContent(MainWindow* window, const YAML::Node& content, const Declaration& scope);
	void applyDocuments(MainWindow* window, const YAML::Node& block, const Declaration& scope);
	void applyPanels(MainWindow* window, const YAML::Node& block, const Declaration& scope, bool bottom);
	QVariantMap zoneOptions(const YAML::Node& content) const;
	QString nativeBaseOf(const Declaration& declaration);
	Declaration* findDeclaration(const QString& requestedName);
	Declaration parseDeclaration(const YAML::Node& define, const QString& fallbackName) const;
	std::optional<YAML::Node> readDefine(const QString& path) const;
	void resolve(Declaration& declaration);
	YAML::Node merge(const YAML::Node& base, const YAML::Node& child) const;

	ClassManager* q;
	Resources* resources;
	CommandController* commandController;

	QString applicationName;
	QString applicationRoot;
	std::map<QString, Declaration> declarations;
	std::map<QString, QString> aliases;
};

QWidget* ClassManager::Private::createViewport(const BootstrapConfig::Application& application) {
	try {
		const auto profile = resources->Directories.get("path.profile");
		if (!profile) {
			throw std::runtime_error("Profile path is not set");
		}

		applicationName = application.name;
		applicationRoot = profile->filePath("applications/" + applicationName);

		const auto mainPath = applicationRoot + QStringLiteral("/main.yaml");
		qDebug() << "ClassManager: loading application description:" << mainPath;

		auto mainDefine = readDefine(mainPath);
		if (!mainDefine) {
			throw std::runtime_error("Cannot load application description: " + mainPath.toStdString());
		}

		Declaration entry = parseDeclaration(*mainDefine, applicationName);
		aliases[entry.name] = entry.name;

		auto inserted = declarations.emplace(entry.name, std::move(entry));
		Declaration& viewportDeclaration = inserted.first->second;
		resolve(viewportDeclaration);

		Declaration* viewport = nullptr;
		if (viewportDeclaration.xtype == application.viewport ||
			viewportDeclaration.name == application.viewport) {
			viewport = &viewportDeclaration;
		}
		else {
			viewport = findDeclarationByType(application.viewport);
		}

		if (!viewport) {
			qWarning() << "ClassManager: viewport type is not defined:" << application.viewport;
			return nullptr;
		}

		return buildViewport(*viewport);
	}
	catch (const std::exception& e) {
		qCritical() << "ClassManager: application load failed:" << e.what();
		return nullptr;
	}
}

ClassManager::Private::Declaration* ClassManager::Private::getDeclaration(const QString& name) {
	if (name.isEmpty()) {
		return nullptr;
	}

	const auto aliasIt = aliases.find(name);
	const auto canonical = (aliasIt != aliases.end()) ? aliasIt->second : name;

	const auto existing = declarations.find(canonical);
	if (existing != declarations.end()) {
		resolve(existing->second);
		return &existing->second;
	}

	return findDeclaration(name);
}

ClassManager::Private::Declaration* ClassManager::Private::findDeclaration(const QString& requestedName) {
	const auto path = classFilePath(applicationRoot, requestedName);
	if (!QFileInfo::exists(path)) {
		return nullptr;
	}

	auto define = readDefine(path);
	if (!define) {
		return nullptr;
	}

	Declaration declaration = parseDeclaration(*define, requestedName);
	aliases[requestedName] = declaration.name;
	aliases[declaration.name] = declaration.name;

	auto inserted = declarations.emplace(declaration.name, std::move(declaration));
	Declaration& stored = inserted.first->second;
	resolve(stored);
	return &stored;
}

ClassManager::Private::Declaration* ClassManager::Private::findDeclarationByType(const QString& type) {
	QStringList names;
	names.reserve(static_cast<int>(declarations.size()));

	for (const auto& entry : declarations) {
		names << entry.first;
	}

	for (const auto& name : names) {
		auto* declaration = getDeclaration(name);
		if (!declaration) {
			continue;
		}

		if (declaration->xtype == type || declaration->name == type) {
			return declaration;
		}
	}

	return nullptr;
}

ClassManager::Private::Declaration* ClassManager::Private::findDeclarationForType(const Declaration& scope, const QString& type) {
	if (!scope.xtype.isEmpty() && scope.xtype == type) {
		return getDeclaration(scope.name);
	}

	for (const auto& className : scope.visibleClasses) {
		auto* declaration = getDeclaration(className);
		if (!declaration) {
			continue;
		}

		if (declaration->xtype == type || declaration->name == type) {
			return declaration;
		}
	}

	return nullptr;
}

QWidget* ClassManager::Private::buildViewport(Declaration& declaration) {
	if (!commandController) {
		qWarning() << "ClassManager: command controller is not available";
		return nullptr;
	}

	const auto nativeType = nativeBaseOf(declaration);
	qDebug() << "ClassManager: building viewport" << declaration.name << "as" << nativeType;

	WidgetBuildHandler handler;
	handler.className = nativeType;
	handler.xtype = declaration.xtype;
	handler.title = declaration.title;
	handler.parent = nullptr;
	handler.context = commandController->globalContext();
	// Зоны оболочки, выключенные описанием, сообщаются фабрике вьюпорта.
	handler.options = zoneOptions(declaration.content);

	auto* widget = commandController->widgets()->build(nativeType, handler);
	if (!widget) {
		qWarning() << "ClassManager: viewport widget is not registered:" << nativeType;
		return nullptr;
	}

	if (auto* window = qobject_cast<MainWindow*>(widget)) {
		applyContent(window, declaration.content, declaration);

		if (!declaration.title.isEmpty()) {
			window->setWindowTitle(declaration.title);
		}
		else if (window->windowTitle().isEmpty()) {
			window->setWindowTitle(QStringLiteral("Red Complex (mp)"));
		}

		window->buildDefaultPanels();
	}

	return widget;
}

void ClassManager::Private::applyContent(MainWindow* window, const YAML::Node& content, const Declaration& scope) {
	if (!content) {
		return;
	}

	if (!content.IsSequence()) {
		qWarning() << "ClassManager: content of" << scope.name << "must be a sequence";
		return;
	}

	for (const auto& block : content) {
		if (!block.IsMap() || block.size() != 1) {
			qWarning() << "ClassManager: unsupported content block in" << scope.name;
			continue;
		}

		const auto blockName = QString::fromStdString(block.begin()->first.Scalar());
		const auto body = block.begin()->second;

		if (blockName == QLatin1String("bottombar")) {
			applyPanels(window, body, scope, true);
		}
		else if (blockName == QLatin1String("sidebar")) {
			applyPanels(window, body, scope, false);
		}
		else if (blockName == QLatin1String("documents")) {
			applyDocuments(window, body, scope);
		}
		else {
			qWarning() << "ClassManager: unknown viewport block:" << blockName;
		}
	}
}

/// Открываемые при загрузке документы: showTabs и список content
/// с тем же описанием элемента, что и у панелей.
void ClassManager::Private::applyDocuments(MainWindow* window, const YAML::Node& block,
	const Declaration& scope) {
	if (!block || !block.IsMap()) {
		qWarning() << "ClassManager: documents block of" << scope.name << "must be a map";
		return;
	}

	// showTabs по умолчанию true: панель открытых документов видна.
	if (const auto showTabs = nodeBool(block, "showTabs")) {
		window->setDocumentTabsVisible(*showTabs);
	}

	const auto items = block["content"];
	if (!items || items.IsNull()) {
		return;
	}

	if (!items.IsSequence()) {
		qWarning() << "ClassManager: documents content of" << scope.name << "must be a sequence";
		return;
	}

	int index = 0;
	for (const auto& item : items) {
		index++;

		if (!item.IsMap()) {
			qWarning() << "ClassManager: document description must be a map";
			continue;
		}

		const auto type = nodeString(item, "xtype");
		if (type.isEmpty()) {
			qWarning() << "ClassManager: document without xtype in" << scope.name;
			continue;
		}

		auto* documentDeclaration = findDeclarationForType(scope, type);
		const auto nativeType = documentDeclaration ? nativeBaseOf(*documentDeclaration) : type;

		auto name = nodeString(item, "name");
		if (name.isEmpty()) {
			name = QStringLiteral("%1-%2").arg(type).arg(index);
		}

		auto title = nodeString(item, "title");
		if (title.isEmpty() && documentDeclaration) {
			title = documentDeclaration->title;
		}
		if (title.isEmpty()) {
			title = name;
		}

		const auto glyph = nodeString(item, "glyph");
		const auto tooltip = nodeString(item, "tooltip");

		auto* registry = commandController ? commandController->widgets() : nullptr;
		if (!registry) {
			qWarning() << "ClassManager: widget registry is not available";
			return;
		}

		WidgetBuildHandler handler;
		handler.className = nativeType;
		handler.xtype = type;
		handler.name = name;
		handler.title = title;
		handler.glyph = glyph;
		handler.parent = window;
		handler.context = commandController ? commandController->globalContext()
			: std::shared_ptr<CommandContext>();

		auto* document = window->createWidget(name, MainWindow::WidgetKind::Document, title, glyph,
			[registry, nativeType, handler]() { return registry->build(nativeType, handler); },
			tooltip, type);
		if (!document) {
			qWarning() << "ClassManager: document is not registered:" << type << "name:" << name;
		}
	}
}

/// Флаги зон оболочки из content вьюпорта: "bottombar.delete" и "sidebar.delete".
/// Ключи совпадают с именами в описании; читает их MainWindow::Options::fromBuildOptions().
QVariantMap ClassManager::Private::zoneOptions(const YAML::Node& content) const {
	QVariantMap options;

	if (!content || !content.IsSequence()) {
		return options;
	}

	for (const auto& block : content) {
		if (!block.IsMap() || block.size() != 1) {
			continue;
		}

		const auto blockName = QString::fromStdString(block.begin()->first.Scalar());
		const auto body = block.begin()->second;
		if (!body || !body.IsMap()) {
			continue;
		}

		if (blockName == QLatin1String("bottombar") || blockName == QLatin1String("sidebar")) {
			if (const auto remove = nodeBool(body, "delete")) {
				options[blockName + QStringLiteral(".delete")] = *remove;
			}
		}
	}

	return options;
}

void ClassManager::Private::applyPanels(MainWindow* window, const YAML::Node& block,
	const Declaration& scope, bool bottom) {
	const auto items = block["content"];
	if (!items || items.IsNull()) {
		// Допустимо: блок задаёт только delete или default.
		return;
	}

	if (!items.IsSequence()) {
		qWarning() << "ClassManager: panels content of" << scope.name << "must be a sequence";
		return;
	}

	const auto zoneName = bottom ? QStringLiteral("bottombar") : QStringLiteral("sidebar");
	const auto kind = bottom ? MainWindow::WidgetKind::BottomPanel : MainWindow::WidgetKind::SidePanel;
	const auto defaultPanel = nodeString(block, "default");
	QStringList registered;
	int index = 0;

	for (const auto& item : items) {
		index++;

		if (!item.IsMap()) {
			qWarning() << "ClassManager: panel description must be a map";
			continue;
		}

		const auto type = nodeString(item, "xtype");
		if (type.isEmpty()) {
			qWarning() << "ClassManager: panel without xtype in" << scope.name;
			continue;
		}

		auto name = nodeString(item, "name");
		if (name.isEmpty()) {
			name = QStringLiteral("%1-%2").arg(type).arg(index);
		}

		if (!window->isZoneEnabled(kind)) {
			// Зона удалена описанием (delete: true): добавление в неё — ошибка.
			qWarning() << "ClassManager:" << zoneName << "is deleted (delete: true),"
				<< "panel cannot be added:" << name << "xtype:" << type;
			continue;
		}

		auto* panelDeclaration = findDeclarationForType(scope, type);
		const auto nativeType = panelDeclaration ? nativeBaseOf(*panelDeclaration) : type;

		auto title = nodeString(item, "title");
		if (title.isEmpty() && panelDeclaration) {
			title = panelDeclaration->title;
		}
		if (title.isEmpty()) {
			title = name;
		}

		WidgetBuildHandler handler;
		handler.className = nativeType;
		handler.xtype = type;
		handler.name = name;
		handler.title = title;
		handler.glyph = nodeString(item, "glyph");
		handler.parent = window;
		handler.context = commandController ? commandController->globalContext()
			: std::shared_ptr<CommandContext>();

		auto* panel = commandController ? commandController->widgets()->build(nativeType, handler) : nullptr;
		if (!panel) {
			qWarning() << "ClassManager: panel is not registered:" << type << "name:" << name;
			continue;
		}

		if (bottom) {
			window->registerBottomPanel(name, handler.glyph, title, panel, nodeString(item, "tooltip"), type);
		}
		else {
			window->registerSidePanel(name, handler.glyph, title, panel, nodeString(item, "tooltip"), type);
		}

		registered << name;
	}

	if (defaultPanel.isEmpty()) {
		return;
	}

	if (!registered.contains(defaultPanel)) {
		qWarning() << "ClassManager: default panel is not registered:" << defaultPanel;
		return;
	}

	if (bottom) {
		window->showBottomPanel(defaultPanel);
	}
	else {
		window->showSidePanel(defaultPanel);
	}
}

QString ClassManager::Private::nativeBaseOf(const Declaration& declaration) {
	QString base = declaration.name;
	const Declaration* current = &declaration;

	while (current && !current->extends.isEmpty()) {
		base = current->extends;
		current = getDeclaration(base);
	}

	return base;
}

ClassManager::Private::Declaration ClassManager::Private::parseDeclaration(
	const YAML::Node& define, const QString& fallbackName) const {
	Declaration declaration;
	declaration.name = nodeString(define, "name");
	if (declaration.name.isEmpty()) {
		declaration.name = fallbackName;
	}

	declaration.xtype = nodeString(define, "xtype");
	declaration.extends = nodeString(define, "extends");
	declaration.title = nodeString(define, "title");
	declaration.requiredClasses = nodeStringList(define, "requires");

	if (define["content"]) {
		declaration.content = YAML::Clone(define["content"]);
	}

	return declaration;
}

std::optional<YAML::Node> ClassManager::Private::readDefine(const QString& path) const {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return std::nullopt;
	}

	YAML::Node root = YAML::Load(file.readAll().toStdString());
	if (!root["define"]) {
		throw std::runtime_error("Description has no define block: " + path.toStdString());
	}

	return root["define"];
}

void ClassManager::Private::resolve(Declaration& declaration) {
	if (declaration.resolved) {
		return;
	}

	if (declaration.resolving) {
		throw std::runtime_error("Cyclic class inheritance: " + declaration.name.toStdString());
	}

	declaration.resolving = true;

	YAML::Node baseContent;
	if (!declaration.extends.isEmpty()) {
		if (auto* base = getDeclaration(declaration.extends)) {
			baseContent = base->content;
			declaration.visibleClasses << base->visibleClasses;
		}
	}

	for (const auto& required : declaration.requiredClasses) {
		auto* requiredDeclaration = getDeclaration(required);
		if (requiredDeclaration) {
			declaration.visibleClasses << requiredDeclaration->name;
			declaration.visibleClasses << requiredDeclaration->visibleClasses;
		}
		else {
			// Нативный класс, зарегистрированный в WidgetRegistry.
			declaration.visibleClasses << required;
		}
	}

	declaration.visibleClasses.removeDuplicates();
	declaration.content = merge(baseContent, declaration.content);
	declaration.resolved = true;
	declaration.resolving = false;
}

YAML::Node ClassManager::Private::merge(const YAML::Node& base, const YAML::Node& child) const {
	if (!base || base.IsNull()) {
		return child ? YAML::Clone(child) : YAML::Node();
	}

	if (!child || child.IsNull()) {
		return YAML::Clone(base);
	}

	if (base.IsMap() && child.IsMap()) {
		YAML::Node result = YAML::Clone(base);

		for (auto it = child.begin(); it != child.end(); it++) {
			const auto key = it->first.Scalar();
			if (result[key]) {
				YAML::Node existing = result[key];
				result[key] = merge(existing, it->second);
			}
			else {
				result[key] = YAML::Clone(it->second);
			}
		}

		return result;
	}

	if (base.IsSequence() && child.IsSequence()) {
		YAML::Node result = YAML::Clone(base);
		std::vector<bool> used(child.size(), false);

		for (std::size_t index = 0; index < result.size(); index++) {
			const auto identity = sequenceIdentity(result[index]);
			int matchIndex = -1;

			if (identity) {
				for (std::size_t childIndex = 0; childIndex < child.size(); childIndex++) {
					if (used[childIndex]) {
						continue;
					}

					if (sequenceIdentity(child[childIndex]) == identity) {
						matchIndex = static_cast<int>(childIndex);
						break;
					}
				}
			}
			else if (index < child.size() && !used[index] && !sequenceIdentity(child[index])) {
				matchIndex = static_cast<int>(index);
			}

			if (matchIndex < 0) {
				continue;
			}

			YAML::Node existing = result[index];
			result[index] = merge(existing, child[static_cast<std::size_t>(matchIndex)]);
			used[static_cast<std::size_t>(matchIndex)] = true;
		}

		for (std::size_t childIndex = 0; childIndex < child.size(); childIndex++) {
			if (!used[childIndex]) {
				result.push_back(YAML::Clone(child[childIndex]));
			}
		}

		return result;
	}

	return YAML::Clone(child);
}

ClassManager::ClassManager(Resources* resources, CommandController* commandController)
	: d(std::make_unique<Private>(this, resources, commandController)) {
}

ClassManager::~ClassManager() = default;

QWidget* ClassManager::createViewport(const BootstrapConfig::Application& application) {
	return d->createViewport(application);
}
