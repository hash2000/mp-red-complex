#include "Launcher/widgets_builder.h"
#include "Content/CodeEditorWidget/code_editor_widget.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/services/fetch_store_db_service.h"
#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
#include "Content/FetchApiModule/widgets/fetch_queries_tree_widget.h"
#include "Content/UsersModule/services/users_registry_service.h"
#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/widgets/login_widget.h"
#include "Content/UsersModule/widgets/register_widget.h"
#include "Content/UsersModule/widgets/user_widget.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/ui/command_console.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/ui/widget_registry.h"

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
	registry->registerFactory({ kMainViewportClass, kMainViewportXtype },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false);
		});

	// Консоль команд: для неё нужен CommandController.
	registry->registerFactory({ "System.Panels.ConsolePanel", "console" },
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
	registry->registerFactory({ "System.Panels.Properties", "properties" },
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
	registry->registerFactory({ "System.Panels.Explorer", "explorer" },
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

	registry->registerFactory({ "Content.Users.LoginWidget", "user-login" },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersService>(servicesFor(type, handler), type, "UsersService");
			return service ? new LoginWidget(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.Users.RegisterWidget", "user-register" },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersRegistryService>(servicesFor(type, handler), type, "UsersRegistryService");
			if (!service) {
				return nullptr;
			}
			auto* widget = new RegisterWidget(service, handler.parent);
			// QDialog по умолчанию является модальным окном, а не страницей панели.
			widget->setWindowFlags(Qt::Widget);
			widget->setModal(false);
			return widget;
		});

	registry->registerFactory({ "Content.Users.UserWidget", "user-profile" },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersService>(servicesFor(type, handler), type, "UsersService");
			return service ? new UserWidget(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.FetchApi.FetchApiWidget", "fetch-api" },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			if (!services || !requiredService<FetchApiService>(services, type, "FetchApiService") ||
				!requiredService<HighlightingPluginManager>(services, type, "HighlightingPluginManager") ||
				!requiredService<FormatterPluginManager>(services, type, "FormatterPluginManager")) {
				return nullptr;
			}
			return new FetchApiWidget(handler.context, handler.parent);
		});

	registry->registerFactory({ "Content.FetchApi.FetchQueriesTreeWidget", "fetch-api-queries-tree" },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<FetchStoreDbService>(servicesFor(type, handler), type, "FetchStoreDbService");
			if (!service) {
				return nullptr;
			}
			auto* widget = new FetchQueriesTreeWidget(service, handler.parent);
			// LazyTreeWidget пока не передаёт parent конструктору QWidget.
			widget->setParent(handler.parent);
			return widget;
		});

	registry->registerFactory({ "Content.CodeEditor.CodeEditorWidget", "code-editor" },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* highlighter = requiredService<HighlightingPluginManager>(services, type, "HighlightingPluginManager");
			auto* formatter = requiredService<FormatterPluginManager>(services, type, "FormatterPluginManager");
			return highlighter && formatter ? new CodeEditorWidget(highlighter, formatter, handler.parent) : nullptr;
		});

	//registry->registerFactory({ "Launcher.WarmupWidget", "warmup" },
	//	[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
	//		return new WarmupWidget(handler.parent);
	//	});

	// Универсальная панель-заглушка (вывод, ошибки, поиск, git и т.п.).
	registry->registerFactory({ "System.Panels.Panel", "panel" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return makePlaceholderPanel(handler);
		});
}
