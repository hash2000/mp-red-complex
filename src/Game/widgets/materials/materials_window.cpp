#include "Game/widgets/materials/materials_window.h"
#include "Game/widgets/materials/material_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeView>
#include <QTableView>
#include <QStandardItemModel>
#include <QCloseEvent>

class MaterialsWindow::Private {
public:
	Private(MaterialsWindow* parent) : q(parent) {}
	MaterialsWindow* q;

	MaterialWidget* materialWidget = nullptr;
	QTabWidget* tabWidget = nullptr;
	QSplitter* mainSplitter = nullptr;
	QSplitter* objectsSplitter = nullptr;

	// Вкладка "Объекты"
	QTreeView* objectTreeView = nullptr;
	QStandardItemModel* objectTreeModel = nullptr;
	QWidget* propertiesWidget = nullptr;

	// Вкладка "Переменные"
	QTableView* variablesTableView = nullptr;
	QStandardItemModel* variablesModel = nullptr;
};

MaterialsWindow::MaterialsWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
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

	// === Первая вкладка: Объекты ===
	QWidget* objectsTab = new QWidget();
	QVBoxLayout* objectsLayout = new QVBoxLayout(objectsTab);
	objectsLayout->setContentsMargins(0, 0, 0, 0);

	// Сплиттер для дерева объектов и свойств (вертикальный)
	d->objectsSplitter = new QSplitter(Qt::Vertical, objectsTab);

	// Дерево объектов
	d->objectTreeView = new QTreeView();
	d->objectTreeModel = new QStandardItemModel(this);
	d->objectTreeView->setModel(d->objectTreeModel);
	d->objectTreeView->setHeaderHidden(false);
	d->objectTreeView->setExpandsOnDoubleClick(true);
	
	// Заголовки для дерева
	d->objectTreeModel->setHorizontalHeaderLabels({"Name", "Type"});

	// Заготовка для добавления ветвей разных типов
	// FolderNode, ShaderNode, TextureNode будут добавлены позже
	QStandardItem* rootItem = d->objectTreeModel->invisibleRootItem();
	
	// Пример заготовки (будет заменено реальными данными)
	QStandardItem* folderPlaceholder = new QStandardItem("Folder (placeholder)");
	folderPlaceholder->setData("FolderNode", Qt::UserRole + 1); // Тип узла
	rootItem->appendRow(folderPlaceholder);

	QStandardItem* shaderPlaceholder = new QStandardItem("Shader (placeholder)");
	shaderPlaceholder->setData("ShaderNode", Qt::UserRole + 1);
	rootItem->appendRow(shaderPlaceholder);

	QStandardItem* texturePlaceholder = new QStandardItem("Texture (placeholder)");
	texturePlaceholder->setData("TextureNode", Qt::UserRole + 1);
	rootItem->appendRow(texturePlaceholder);

	d->objectsSplitter->addWidget(d->objectTreeView);

	// Свойства (нижняя часть) - виджет будет добавлен пользователем
	d->propertiesWidget = new QWidget();
	d->propertiesWidget->setMinimumHeight(150);
	d->objectsSplitter->addWidget(d->propertiesWidget);

	objectsLayout->addWidget(d->objectsSplitter);
	d->tabWidget->addTab(objectsTab, "Объекты");

	// === Вторая вкладка: Переменные ===
	QWidget* variablesTab = new QWidget();
	QVBoxLayout* variablesLayout = new QVBoxLayout(variablesTab);
	variablesLayout->setContentsMargins(0, 0, 0, 0);

	d->variablesTableView = new QTableView();
	d->variablesModel = new QStandardItemModel(this);
	d->variablesTableView->setModel(d->variablesModel);
	
	// Настройка таблицы переменных для шейдера
	d->variablesModel->setHorizontalHeaderLabels({"Name", "Type", "Value"});
	d->variablesTableView->horizontalHeader()->setStretchLastSection(true);
	d->variablesTableView->setAlternatingRowColors(true);

	variablesLayout->addWidget(d->variablesTableView);
	d->tabWidget->addTab(variablesTab, "Переменные");

	// Добавляем tab widget в правую часть сплиттера
	d->mainSplitter->addWidget(d->tabWidget);

	// Устанавливаем размеры сплиттеров
	d->mainSplitter->setStretchFactor(0, 1); // MaterialWidget занимает больше места
	d->mainSplitter->setStretchFactor(1, 0);
	d->mainSplitter->setSizes({width() - 300, 300});

	d->objectsSplitter->setStretchFactor(0, 1); // Дерево занимает больше места
	d->objectsSplitter->setStretchFactor(1, 0);
	d->objectsSplitter->setSizes({height() - 200, 200});

	// Устанавливаем центральный виджет
	setWidget(d->mainSplitter);
}

bool MaterialsWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	// Обработка команд (пока пусто, может быть расширено)
	Q_UNUSED(commandName);
	Q_UNUSED(args);
	Q_UNUSED(context);
	return false;
}

void MaterialsWindow::closeEvent(QCloseEvent* closeEvent) {
	MdiChildWindow::closeEvent(closeEvent);
}
