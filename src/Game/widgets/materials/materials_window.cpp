#include "Game/widgets/materials/materials_window.h"
#include "Game/widgets/materials/material_widget.h"
#include "Game/widgets/materials/material_objects.h"
#include "Game/widgets/materials/material_variables.h"
#include <QHeaderView>
#include <QSplitter>

class MaterialsWindow::Private {
public:
	Private(MaterialsWindow* parent) : q(parent) {}
	MaterialsWindow* q;

	MaterialsService* materialsService = nullptr;
	MaterialWidget* materialWidget = nullptr;
	QTabWidget* tabWidget = nullptr;
	QSplitter* mainSplitter = nullptr;

	// Вкладка "Объекты"
	MaterialObjects* objectTree = nullptr;

	// Вкладка "Переменные"
	MaterialVariables* variablesTree = nullptr;
};

MaterialsWindow::MaterialsWindow(MaterialsService* materialsService, const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
	d->materialsService = materialsService;
	setupUi();
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
}

bool MaterialsWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	// Обработка команд (пока пусто, может быть расширено)
	Q_UNUSED(commandName);
	Q_UNUSED(args);
	Q_UNUSED(context);
	return true;
}
