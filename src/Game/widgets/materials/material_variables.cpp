#include "Game/widgets/materials/material_variables.h"
#include <QSplitter>
#include <QTableView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QHeaderView>

class MaterialVariables::Private {
public:
	Private(MaterialVariables* parent) : q(parent) {}
	MaterialVariables* q;
	QSplitter* objectsSplitter = nullptr;

	// Вкладка "Объекты"
	QTableView* variablesTableView = nullptr;
	QStandardItemModel* variablesModel = nullptr;
};

MaterialVariables::MaterialVariables(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	setupUI();
}

MaterialVariables::~MaterialVariables() = default;

void MaterialVariables::setupUI() {
	auto* variablesLayout = new QVBoxLayout(this);
	variablesLayout->setContentsMargins(0, 0, 0, 0);

	d->variablesTableView = new QTableView();
	d->variablesModel = new QStandardItemModel(this);
	d->variablesTableView->setModel(d->variablesModel);

	// Настройка таблицы переменных для шейдера
	d->variablesModel->setHorizontalHeaderLabels({ "Name", "Type", "Value" });
	d->variablesTableView->horizontalHeader()->setStretchLastSection(true);
	d->variablesTableView->setAlternatingRowColors(true);

	variablesLayout->addWidget(d->variablesTableView);
}
