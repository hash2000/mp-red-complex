#include "Launcher/widgets_builder.h"
#include "Content/TextEditorModule/documents/code_editor_document.h"
#include "Content/TextEditorModule/documents/code_editor_document_metadata.h"
#include "Content/TextEditorModule/documents/paged_text_document.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin_manager.h"
#include "Content/DatabaseModule/documents/database_backup_document.h"
#include "Content/DatabaseModule/documents/database_backup_document_metadata.h"
#include "Content/DatabaseModule/services/database_backup_service.h"
#include "Content/FetchApiModule/documents/fetch_api_document.h"
#include "Content/FetchApiModule/documents/fetch_api_document_metadata.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/SystemRouter/documents/system_router_node_properties_document.h"
#include "Content/SystemRouter/documents/system_router_node_view_document.h"
#include "Content/SystemRouter/documents/system_router_tree_document.h"
#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/services/system_router_service.h"
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
#include "Libs/Engine/ui/main_window/system_options_document.h"
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

	// Регистрация описывает тип: класс, xtype, признак синглтона и подпись.
	// Оформление вкладок по умолчанию (стандартный глиф и цвет) задаёт тема —
	// assets/themes/{theme}/documents.yaml.

	// Главное окно приложения. Панели создаются декларативно из yaml-описания,
	// поэтому встроенные панели по умолчанию не собираются.
	// Зоны, удалённые описанием (bottombar/sidebar delete: true), не создаются.
	registry->registerFactory({ kMainViewportClass, kMainViewportXtype, /*singletone*/ true,
		QStringLiteral("Оболочка") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent,
				/*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});

	// Консоль команд: для неё нужен CommandController.
	registry->registerFactory({ "System.Panels.ConsolePanel", "console", /*singletone*/ false,
		QStringLiteral("Консоль") },
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
		QStringLiteral("Свойства") },
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
		QStringLiteral("Проводник") },
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

	registry->registerFactory({ "Content.Users.Login", "user-login", /*singletone*/ true,
		QStringLiteral("Вход") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersService>(servicesFor(type, handler), type, "UsersService");
			return service ? new LoginDocument(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.Users.Register", "user-register", /*singletone*/ true,
		QStringLiteral("Регистрация") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersRegistryService>(servicesFor(type, handler), type, "UsersRegistryService");
			return service ? new RegisterDocument(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.Users.User", "user-profile", /*singletone*/ true,
		QStringLiteral("Профиль") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<UsersService>(servicesFor(type, handler), type, "UsersService");
			return service ? new UserDocument(service, handler.parent) : nullptr;
		});

	registry->registerFactory({ "Content.FetchApi.FetchApi", "fetch-api", /*singletone*/ false,
		QStringLiteral("Fetch API") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* fetchService = requiredService<FetchApiService>(services, type, "FetchApiService");
			if (!fetchService) {
				return nullptr;
			}
			// FetchApi не зависит от редактора кода: ответ — текстовое поле.
			return new FetchApiDocument(fetchService, handler.parent);
		});

	// Переменные, из которых документ восстанавливает запрос после открытия.
	registry->registerDocumentMetadata(QStringLiteral("Content.FetchApi.FetchApi"),
		std::make_unique<FetchApiDocumentMetadata>());

	registry->registerFactory({ "Content.SystemRouter.SystemRouterTree", "system-router",
		/*singletone*/ true, QStringLiteral("Системный маршрутизатор") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* service = requiredService<SystemRouterService>(services, type, "SystemRouterService");
			auto* commandService = requiredService<SystemRouterCommandService>(services, type,
				"SystemRouterCommandService");
			auto* blobService = requiredService<SystemRouterBlobService>(services, type,
				"SystemRouterBlobService");
			return service ? new SystemRouterTreeDocument(service, commandService, blobService,
				handler.parent) : nullptr;
		});

	// Параметры узла: обычный документ, один на узел (id задаёт вызывающая сторона).
	registry->registerFactory({ "Content.SystemRouter.SystemRouterNodeProperties",
		"system-router-node-properties", /*singletone*/ false,
		QStringLiteral("Параметры узла") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* service = requiredService<SystemRouterService>(services, type, "SystemRouterService");
			auto* blobService = requiredService<SystemRouterBlobService>(services, type,
				"SystemRouterBlobService");
			return service ? new SystemRouterNodePropertiesDocument(service, blobService, {},
				handler.parent) : nullptr;
		});

	// Просмотр узла: только чтение параметров, один документ на узел.
	registry->registerFactory({ "Content.SystemRouter.SystemRouterNodeView",
		"system-router-node-view", /*singletone*/ false,
		QStringLiteral("Просмотр узла") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* service = requiredService<SystemRouterService>(services, type, "SystemRouterService");
			auto* commandService = requiredService<SystemRouterCommandService>(services, type,
				"SystemRouterCommandService");
			return service ? new SystemRouterNodeViewDocument(service, commandService, {},
				handler.parent) : nullptr;
		});

	// Резервное копирование баз данных, зарегистрированных в контекстах
	// `.store.<alias>`. Документ-синглтон: одновременно открыт один менеджер
	// копий, повторное создание активирует его.
	registry->registerFactory({ "Content.Database.DatabaseBackup", "database-backup",
		/*singletone*/ true, QStringLiteral("Резервное копирование баз данных") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* service = requiredService<DatabaseBackupService>(servicesFor(type, handler), type,
				"DatabaseBackupService");
			return service ? new DatabaseBackupDocument(service, handler.parent) : nullptr;
		});

	// Диалог создания документа показывает обязательную переменную `store`:
	// по ней документ восстанавливает выбранное хранилище.
	registry->registerDocumentMetadata(QStringLiteral("Content.Database.DatabaseBackup"),
		std::make_unique<DatabaseBackupDocumentMetadata>());

	// Редактор кода — обычный документ: экземпляров может быть несколько.
	registry->registerFactory({ "Content.TextEditor.CodeEditor", "code-editor",
		/*singletone*/ false, QStringLiteral("Редактор кода") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* highlighter = requiredService<HighlightingPluginManager>(services, type, "HighlightingPluginManager");
			auto* formatter = requiredService<FormatterPluginManager>(services, type, "FormatterPluginManager");
			return highlighter && formatter ? new CodeEditorDocument(highlighter, formatter, handler.parent) : nullptr;
		});

	// Обязательные переменные документа-редактора: путь к файлу.
	registry->registerDocumentMetadata(QStringLiteral("Content.TextEditor.CodeEditor"),
		std::make_unique<CodeEditorDocumentMetadata>());

	// Текстовый редактор (text-edit): виртуальный редактор для больших
	// файлов, страницы читаются с диска, в памяти только индекс строк и кэш.
	registry->registerFactory({ "Content.TextEditor.PagedText", "text-editor",
		/*singletone*/ false, QStringLiteral("Текстовый редактор") },
		[](const QString& type, const WidgetBuildHandler& handler) -> QWidget* {
			auto* services = servicesFor(type, handler);
			auto* highlighter = requiredService<HighlightingPluginManager>(
				services, type, "HighlightingPluginManager");
			auto* formatter = requiredService<FormatterPluginManager>(
				services, type, "FormatterPluginManager");
			return highlighter && formatter
				? new PagedTextDocument(highlighter, formatter, handler.parent) : nullptr;
		});
	registry->registerDocumentMetadata(QStringLiteral("Content.TextEditor.PagedText"),
		std::make_unique<PagedTextDocumentMetadata>());

	//registry->registerFactory({ "Launcher.WarmupWidget", "warmup" },
	//	[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
	//		return new WarmupWidget(handler.parent);
	//	});

	// Документ системных настроек. Роль DocumentRule::SystemOptions даёт ему
	// контекст `.options` в режиме Mutable: только он может менять системные
	// параметры глобального контекста.
	registry->registerFactory({ SystemOptionsDocument::className(), SystemOptionsDocument::xtype(),
		/*singletone*/ true, QStringLiteral("Системные настройки"),
		DocumentRule::SystemOptions },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new SystemOptionsDocument(handler.parent);
		});

	// Универсальная панель-заглушка (вывод, ошибки, поиск, git и т.п.).
	registry->registerFactory({ "System.Panels.Panel", "panel", /*singletone*/ false,
		QStringLiteral("Панель") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return makePlaceholderPanel(handler);
		});
}
