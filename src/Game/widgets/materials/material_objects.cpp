#include "Game/widgets/materials/material_objects.h"
#include <QSplitter>
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>

class MaterialObjects::Private {
public:
	Private(MaterialObjects* parent) : q(parent) {}
	MaterialObjects* q;
	QSplitter* objectsSplitter = nullptr;

	// Вкладка "Объекты"
	QTreeView* objectTreeView = nullptr;
	QStandardItemModel* objectTreeModel = nullptr;
	QWidget* propertiesWidget = nullptr;

};

MaterialObjects::MaterialObjects(QWidget* parent)
: QWidget(parent)
, d(std::make_unique<Private>(this)) {
	setupUI();
}

MaterialObjects::~MaterialObjects() = default;

void MaterialObjects::setupUI() {
	auto* objectsLayout = new QVBoxLayout(this);
	objectsLayout->setContentsMargins(0, 0, 0, 0);

	// Сплиттер для дерева объектов и свойств (вертикальный)
	d->objectsSplitter = new QSplitter(Qt::Vertical);

	// Дерево объектов
	d->objectTreeView = new QTreeView();
	d->objectTreeModel = new QStandardItemModel(this);
	d->objectTreeView->setModel(d->objectTreeModel);
	d->objectTreeView->setHeaderHidden(false);
	d->objectTreeView->setExpandsOnDoubleClick(true);

	// Заголовки для дерева
	d->objectTreeModel->setHorizontalHeaderLabels({ "Name", "Type" });

	// Заготовка для добавления ветвей разных типов
	// FolderNode, ShaderNode, TextureNode будут добавлены позже
	auto* rootItem = d->objectTreeModel->invisibleRootItem();

	// Пример заготовки (будет заменено реальными данными)
	auto* folderPlaceholder = new QStandardItem("Folder (placeholder)");
	folderPlaceholder->setData("FolderNode", Qt::UserRole + 1); // Тип узла
	rootItem->appendRow(folderPlaceholder);

	auto* shaderPlaceholder = new QStandardItem("Shader (placeholder)");
	shaderPlaceholder->setData("ShaderNode", Qt::UserRole + 1);
	rootItem->appendRow(shaderPlaceholder);

	auto* texturePlaceholder = new QStandardItem("Texture (placeholder)");
	texturePlaceholder->setData("TextureNode", Qt::UserRole + 1);
	rootItem->appendRow(texturePlaceholder);

	d->objectsSplitter->addWidget(d->objectTreeView);

	// Свойства (нижняя часть) - виджет будет добавлен пользователем
	d->propertiesWidget = new QWidget();
	d->propertiesWidget->setMinimumHeight(150);
	d->objectsSplitter->addWidget(d->propertiesWidget);

	d->objectsSplitter->setStretchFactor(0, 1); // Дерево занимает больше места
	d->objectsSplitter->setStretchFactor(1, 0);
	d->objectsSplitter->setSizes({ height() - 200, 200 });

	objectsLayout->addWidget(d->objectsSplitter);
}
