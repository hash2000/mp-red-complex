#include "Launcher/widgets_builder.h"
#include "Content/CodeEditorWidget/documents/code_editor_document.h"
#include "Content/CodeEditorWidget/documents/code_editor_document_metadata.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/FetchApiModule/documents/fetch_api_document.h"
#include "Content/FetchApiModule/documents/fetch_queries_tree_document.h"
#include "Content/FetchApiModule/services/fetch_queries_service.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/SystemMenu/documents/system_menu_tree_document.h"
#include "Content/SystemMenu/services/system_menu_service.h"
#include "Content/UsersModule/documents/login_document.h"
#include "Content/UsersModule/documents/register_document.h"
#include "Content/UsersModule/documents/user_document.h"
#include "Content/UsersModule/services/users_registry_service.h"
#include "Content/UsersModule/services/users_service.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/ui/command_console.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"

#include <QDebug>
#include <QDir>
#include <QFileSystemModel>
#include <QLabel>
#include <QTreeView>

namespace {

constexpr const char* kMainViewportClass = "System.Viewport.MainForm";
constexpr const char* kMainViewportXtype = "mainform";

MainWindow* findMainWindow(QWidget* widget) {
	for (auto* current = widget; current; current = current->parentWidget()) {
		if (auto* window = qobject_cast<MainWindow*>(current)) {
			return window;
		}
	}
	return nullptr;
}

ServicesRegistry* servicesFor(const QString& type, const WidgetBuildHandler& handler) {
	if (!handler.context || !handler.context->commandController() ||
		!handler.context->commandController()->services()) {
		qWarning() << "WidgetsBuilder: command context or services unavailable for" << type;
		return nullptr;
	}
	return handler.context->commandController()->services();
}

template<typename T>
T* requiredService(ServicesRegistry* services, const QString& type, const char* name) {
	auto* service = services ? services->get<T>() : nullptr;
	if (!service && services) {
		qWarning() << "WidgetsBuilder: service" << name << "unavailable for" << type;
	}
	return service;
}

/// Заглушка для панелей, у которых пока нет собственного виджета.
QWidget* makePlaceholderPanel(const WidgetBuildHandler& handler) {
	const auto text = handler.title.isEmpty() ? handler.xtype : handler.title;

	auto* label = new QLabel(text, handler.parent);
	label->setObjectName(QStringLiteral("EmptyHint"));
	label->setAlignment(Qt::AlignCenter);
	label->setWordWrap(true);
	return label;
}

} // namespace

class WidgetsBuilder::Private {
public:
	Private(WidgetsBuilder* parent, CommandController* commandController)
		: q(parent)
		, commandController(commandController) {
	}

	WidgetsBuilder* q;
	CommandController* commandController;

	void registerAll();
};

WidgetsBuilder::WidgetsBuilder(CommandController* commandController)
	: d(std::make_unique<Private>(this, commandController)) {
	d->registerAll();
}

WidgetsBuilder::~WidgetsBuilder() = default;

void WidgetsBuilder::Private::registerAll() {
	if (!commandController || !commandController->widgets()) {
		qWarning() << "WidgetsBuilder: widget registry is not available";
		return;
	}

	auto* registry = commandController->widgets();

	// Главное окно приложения. Панели создаются декларативно из yaml-описания,
	// поэтому встроенные панели по умолчанию не собираются.
	// Зоны, удалённые описанием (bottombar/sidebar delete: true), не создаются.
	registry->registerFactory({ kMainViewportClass, kMainViewportXtype, /*singletone*/ true,
		QStringLiteral("🖥"), QStringLiteral("Оболочка") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent,
				/*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});

	// Консоль команд: для неё нужен CommandController.
	registry->registerFactory({ "System.Panels.ConsolePanel", "console", /*singletone*/ false,
		QStringLiteral("🖥"), QStringLiteral("Консоль") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			if (!handler.context) {
				return nullptr;
			}

			auto* controller = handler.context->commandController();
			if (!controller) {
				return nullptr;
			}

			return new CommandConsole(controller, handler.parent);
		});

	// Каждый вызов создаёт отдельную панель, следящую за активным документом окна.
	// Документ-синглтон: в приложении только одна панель свойств.
	registry->registerFactory({ "System.Panels.Properties", "properties", /*singletone*/ true,
		QStringLiteral("🛠"), QStringLiteral("Свойства") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			auto* properties = new PropertiesPanel(handler.parent);
			if (auto* window = findMainWindow(handler.parent)) {
				auto* area = window->documentArea();
				properties->setDocument(area->currentDocument());
				QObject::connect(area, &DocumentArea::currentDocumentChanged,
					properties, &PropertiesPanel::setDocument);
				QObject::connect(area, &DocumentArea::documentAboutToClose, properties,
					[properties](DocumentWidget* doc) {
						if (properties->document() == doc) {
							properties->setDocument(nullptr);
						}
					});
			}
			return properties;
		});

	// Проводник по файловой системе; двойной клик открывает документ в окне.
	// Документ-синглтон: второй проводник не создаётся, а активируется первый.
	registry->registerFactory({ "System.Panels.Explorer", "explorer", /*singletone*/ true,
		QStringLiteral("📁"), QStringLiteral("Проводник") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			auto* tree = new QTreeView(handler.parent);
			auto* model = new QFileSystemModel(tree);
			model->setRootPath(QDir::currentPath());

			tree->setModel(model);
			tree->setRootIndex(model->index(QDir::currentPath()));
			tree->setHeaderHidden(true);

			for (int column = 1; column < model->columnCount(); column++) {
				tree->hideColumn(column);
			}

			QObject::connect(tree, &QTreeView::doubleClicked, tree,
				[tree, model](const QModelIndex& index) {
					if (!index.isValid() || model->isDir(index)) {
						return;
					}
					if (auto* window = findMainWindow(tree)) {
						window->openFile(model->filePath(index));
					}
				});

			return tree;
		});

	registry->registerFactory({ "Content.Users.LoginDocument", "user-login", /*singletone*/ true,
		QStringLiteral("🔑"), QStringLiteral("Вход") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersService>(servicesFor(type, handler), type, "UsersService");
			return service ? new LoginDocument(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.Users.RegisterDocument", "user-register", /*singletone*/ true,
		QStringLiteral("📝"), QStringLiteral("Регистрация") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersRegistryService>(servicesFor(type, handler), type, "UsersRegistryService");
			return service ? new RegisterDocument(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.Users.UserDocument", "user-profile", /*singletone*/ true,
		QStringLiteral("👤"), QStringLiteral("Профиль") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersService>(servicesFor(type, handler), type, "UsersService");
			return service ? new UserDocument(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.FetchApi.FetchApiDocument", "fetch-api", /*singletone*/ true,
		QStringLiteral("🌐"), QStringLiteral("Fetch API") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			if (!services || !requiredService<FetchApiService>(services, type, "FetchApiService") ||
				!requiredService<HighlightingPluginManager>(services, type, "HighlightingPluginManager") ||
				!requiredService<FormatterPluginManager>(services, type, "FormatterPluginManager")) {
				return nullptr;
			}
			return new FetchApiDocument(handler.context, handler.parent);
		});

	registry->registerFactory({ "Content.FetchApi.FetchQueriesTreeDocument", "fetch-api-queries-tree",
		/*singletone*/ true, QStringLiteral("🌳"), QStringLiteral("Запросы Fetch API") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<FetchQueriesService>(servicesFor(type, handler), type, "FetchQueriesService");
			return service ? new FetchQueriesTreeDocument(service->dataProvider(), handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.SystemMenu.SystemMenuTreeDocument", "system-menu",
		/*singletone*/ true, QStringLiteral("🗂"), QStringLiteral("Системное меню") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<SystemMenuService>(servicesFor(type, handler), type, "SystemMenuService");
			return service ? new SystemMenuTreeDocument(service, handler.parent) : nullptr;
		});

	// Редактор кода — обычный документ: экземпляров может быть несколько.
	registry->registerFactory({ "Content.CodeEditor.CodeEditorDocument", "code-editor",
		/*singletone*/ false, QStringLiteral("📝"), QStringLiteral("Редактор кода") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* highlighter = requiredService<HighlightingPluginManager>(services, type, "HighlightingPluginManager");
			auto* formatter = requiredService<FormatterPluginManager>(services, type, "FormatterPluginManager");
			return highlighter && formatter ? new CodeEditorDocument(highlighter, formatter, handler.parent) : nullptr;
		});

	// Обязательные переменные документа-редактора: путь к файлу.
	registry->registerDocumentMetadata(QStringLiteral("Content.CodeEditor.CodeEditorDocument"),
		std::make_unique<CodeEditorDocumentMetadata>());

	//registry->registerFactory({ "Launcher.WarmupWidget", "warmup" },
	//	[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
	//		return new WarmupWidget(handler.parent);
	//	});

	// Универсальная панель-заглушка (вывод, ошибки, поиск, git и т.п.).
	registry->registerFactory({ "System.Panels.Panel", "panel", /*singletone*/ false,
		QStringLiteral("🗔"), QStringLiteral("Панель") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return makePlaceholderPanel(handler);
		});
}
