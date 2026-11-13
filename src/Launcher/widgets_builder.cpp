#include "Launcher/widgets_builder.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/ui/command_console.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/ui/widget_registry.h"

#include <QDir>
#include <QFileSystemModel>
#include <QLabel>
#include <QTreeView>

namespace {

constexpr const char* kMainViewportClass = "System.Viewport.MainForm";
constexpr const char* kMainViewportXtype = "mainform";

/// Заглушка для панелей, у которых пока нет собственного виджета.
QWidget* makePlaceholderPanel(const WidgetBuildHandler& handler) {
	const auto text = handler.title.isEmpty() ? handler.xtype : handler.title;

	auto* label = new QLabel(text);
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
			return new MainWindow(handler.context, nullptr, /*buildDefaultPanels*/ false);
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

			return new CommandConsole(controller);
		});

	// Панель свойств уже создана внутри MainWindow и привязана к документам.
	registry->registerFactory({ "System.Panels.Properties", "properties" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			if (auto* window = qobject_cast<MainWindow*>(handler.parent)) {
				if (auto* properties = window->propertiesPanel()) {
					return properties;
				}
			}

			return new PropertiesPanel;
		});

	// Проводник по файловой системе; двойной клик открывает документ в окне.
	registry->registerFactory({ "System.Panels.Explorer", "explorer" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			auto* tree = new QTreeView;
			auto* model = new QFileSystemModel(tree);
			model->setRootPath(QDir::currentPath());

			tree->setModel(model);
			tree->setRootIndex(model->index(QDir::currentPath()));
			tree->setHeaderHidden(true);

			for (int column = 1; column < model->columnCount(); column++) {
				tree->hideColumn(column);
			}

			if (auto* window = qobject_cast<MainWindow*>(handler.parent)) {
				QObject::connect(tree, &QTreeView::doubleClicked, tree,
					[window, model](const QModelIndex& index) {
						if (!index.isValid() || model->isDir(index)) {
							return;
						}

						window->openFile(model->filePath(index));
					});
			}

			return tree;
		});

	// Универсальная панель-заглушка (вывод, ошибки, поиск, git и т.п.).
	registry->registerFactory({ "System.Panels.Panel", "panel" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return makePlaceholderPanel(handler);
		});
}
