#include "Launcher/widgets/materials/materials_window.h"
#include "Content/MaterialsModule/widgets/material_widget.h"
#include "Content/MaterialsModule/widgets/material_objects.h"
#include "Content/MaterialsModule/widgets/material_variables.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/controllers.h"

#include <QHeaderView>
#include <QSplitter>

class MaterialsWindow::Private {
public:
	Private(MaterialsWindow* parent) : q(parent) {}
	MaterialsWindow* q;


	IApplicationCommands* applicationCommands = nullptr;
	MaterialsService* materialsService = nullptr;
	MaterialWidget* materialWidget = nullptr;
	QTabWidget* tabWidget = nullptr;
	QSplitter* mainSplitter = nullptr;

	// Вкладка "Объекты"
	MaterialObjects* objectTree = nullptr;

	// Вкладка "Переменные"
	MaterialVariables* variablesTree = nullptr;
};

MaterialsWindow::MaterialsWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
}

MaterialsWindow::~MaterialsWindow() = default;

void MaterialsWindow::setupUi() {
	// Создаем главный сплиттер (горизонтальный)
	d->mainSplitter = new QSplitter(Qt::Horizontal, this);

	// Создаем MaterialWidget - центральная часть
	d->materialWidget = new MaterialWidget(this);
	d->mainSplitter->addWidget(d->materialWidget);

	// Создаем правую панель с параметрами (tab widget)
	d->tabWidget = new QTabWidget(this);

	d->objectTree = new MaterialObjects(d->materialsService);
	d->tabWidget->addTab(d->objectTree, "Объекты");

	d->variablesTree = new MaterialVariables(this);
	d->tabWidget->addTab(d->variablesTree, "Переменные");


	d->mainSplitter->addWidget(d->tabWidget);
	d->mainSplitter->setStretchFactor(0, 1);
	d->mainSplitter->setStretchFactor(1, 0);
	d->mainSplitter->setSizes({width() - 300, 300});

	setWidget(d->mainSplitter);

	connect(d->objectTree, &MaterialObjects::editMaterialFile, this, &MaterialsWindow::onEditMaterialFile);
}

bool MaterialsWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	// Обработка команд (пока пусто, может быть расширено)
	if (commandName == "create") {
		auto services = context->services();
		d->materialsService = services->materialsService();
		d->applicationCommands = context->applicationController();
		setupUi();
		return true;
	}

	return false;
}

void MaterialsWindow::onEditMaterialFile(MaterialObjectTypes type, const QString& path) {
	if (!d->applicationCommands) {
		return;
	}

	const auto cmdPath = QString("path:%1").arg(path);
	const auto cmdId = QString("id:%1").arg(path);

	d->applicationCommands->executeCommandByName("window-create",
		QStringList{ "target:code-editor", cmdPath, cmdId });
}


