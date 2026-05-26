#include "Game/widgets/materials/material_objects.h"
#include "Game/widgets/materials/material_objects/material_object_menu_actions.h"
#include "Game/i_app_commands.h"
#include "Game/widgets/materials/material_objects/material_object_node.h"
#include "ApplicationLayer/materials/materials_service.h"
#include "ApplicationLayer/materials/material_mime_data.h"
#include "BaseWidgets/properties/properties_list_widget.h"

#include <QSplitter>
#include <QTreeView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QToolButton>
#include <QMenu>

class MaterialObjects::Private {
public:
	Private(MaterialObjects* parent) : q(parent) {}
	MaterialObjects* q;

	MaterialsService* materialsService = nullptr;

	QSplitter* objectsSplitter = nullptr;

	// Вкладка "Объекты"
	QTreeView* objectTreeView = nullptr;
	QStandardItemModel* objectTreeModel = nullptr;
	PropertiesListWidget* propertiesWidget = nullptr;
	QStandardItem* rootMaterialNode = nullptr;
	QModelIndex selection;
};

MaterialObjects::MaterialObjects(
	MaterialsService* materialsService,
	QWidget* parent)
: QWidget(parent)
, d(std::make_unique<Private>(this)) {
	d->materialsService = materialsService;
	setupUI();
}

MaterialObjects::~MaterialObjects() = default;

void MaterialObjects::setupUI() {
	auto* objectsLayout = new QVBoxLayout(this);
	objectsLayout->setContentsMargins(0, 0, 0, 0);
	objectsLayout->setSpacing(0);

	// === Панель инструментов (фиксированная высота 40px) ===
	auto* toolbar = new QWidget(this);
	toolbar->setFixedHeight(30);
	toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	auto* toolbarLayout = new QHBoxLayout(toolbar);
	toolbarLayout->setContentsMargins(4, 2, 4, 2);
	toolbarLayout->setSpacing(4);
	// Кнопки выравниваются по левому краю (как в VS) — stretch только в конце

	// Общий стиль для кнопок: квадратные, только символ, без текста
	const QString symbolButtonStyle = R"(
        QToolButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 3px;
            padding: 2px;
            font-size: 12px;
            min-width: 20px;
            max-width: 20px;
            min-height: 20px;
            max-height: 20px;
        }
        QToolButton:hover {
            border: 1px solid rgba(255, 255, 255, 0.3);
            background: rgba(255, 255, 255, 0.08);
        }
        QToolButton:pressed {
            background: rgba(255, 255, 255, 0.15);
        }
        QToolButton::menu-indicator {
            image: none;
        }
        /* Скрываем текст, оставляем только символ */
        QToolButton[text=""] {
            padding: 6px;
        }
    )";

	// --- Кнопка "Добавить" с меню ---
	auto* addBtn = new QToolButton(toolbar);
	addBtn->setToolTip("Добавить");
	addBtn->setText("➕");  // UTF-8 символ
	addBtn->setPopupMode(QToolButton::InstantPopup);
	addBtn->setToolButtonStyle(Qt::ToolButtonTextOnly); // Текст = символ
	addBtn->setStyleSheet(symbolButtonStyle);

	auto* addMenu = new QMenu(addBtn);
	addMenu->addAction("🎨 Вершинный шейдер");
	addMenu->addAction("🔷 Фрагментный шейдер");
	addMenu->addSeparator();
	addMenu->addAction("📁 Директория");
	addMenu->addAction("🖼️ Текстура");
	addBtn->setMenu(addMenu);

	// Обработка выбора из меню
	connect(addMenu->actions()[0], &QAction::triggered, this, [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::VertexShader); });
	connect(addMenu->actions()[1], &QAction::triggered, this, [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::FragmentShader); });
	connect(addMenu->actions()[3], &QAction::triggered, this, [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::Directory); });
	connect(addMenu->actions()[4], &QAction::triggered, this, [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::Texture); });

	toolbarLayout->addWidget(addBtn);

	// Разделитель (опционально, как в VS)
	auto* separator = new QFrame(toolbar);
	separator->setFrameShape(QFrame::VLine);
	separator->setFrameShadow(QFrame::Sunken);
	separator->setFixedWidth(1);
	separator->setStyleSheet("background: rgba(255,255,255,0.15);");
	toolbarLayout->addWidget(separator);

	// --- Кнопка "Обновить" ---
	auto* refreshBtn = new QToolButton(toolbar);
	refreshBtn->setToolTip("Обновить");
	refreshBtn->setText("🔄");
	refreshBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
	refreshBtn->setStyleSheet(symbolButtonStyle);
	connect(refreshBtn, &QToolButton::clicked, this, &MaterialObjects::refreshRequested);
	toolbarLayout->addWidget(refreshBtn);

	// --- Кнопка "Загрузить" ---
	auto* loadBtn = new QToolButton(toolbar);
	loadBtn->setToolTip("Загрузить");
	loadBtn->setText("📥");
	loadBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
	loadBtn->setStyleSheet(symbolButtonStyle);
	connect(loadBtn, &QToolButton::clicked, this, &MaterialObjects::loadRequested);
	toolbarLayout->addWidget(loadBtn);

	// --- Кнопка "Сохранить" ---
	auto* saveBtn = new QToolButton(toolbar);
	saveBtn->setToolTip("Сохранить");
	saveBtn->setText("💾");
	saveBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
	saveBtn->setStyleSheet(symbolButtonStyle);
	connect(saveBtn, &QToolButton::clicked, this, &MaterialObjects::saveRequested);
	toolbarLayout->addWidget(saveBtn);

	toolbarLayout->addStretch(); // Прижимаем всё влево, остаток — пустое пространство

	objectsLayout->addWidget(toolbar);
	// === Конец панели инструментов ===

	// Сплиттер для дерева и свойств
	d->objectsSplitter = new QSplitter(Qt::Vertical);

	d->objectTreeView = new QTreeView();
	d->objectTreeModel = new QStandardItemModel(this);
	d->objectTreeView->setModel(d->objectTreeModel);
	d->objectTreeView->setHeaderHidden(false);
	d->objectTreeView->setExpandsOnDoubleClick(true);
	d->objectTreeView->setRootIsDecorated(false);
	d->objectTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	d->objectTreeModel->setHorizontalHeaderLabels({ "Name" });

	MaterialObjectNode node(d->objectTreeModel->invisibleRootItem());
	d->rootMaterialNode = MaterialObjectNode::appendNode(
		d->objectTreeModel->invisibleRootItem(),
		"Материалы",
		MaterialObjectTypes::MaterialRoot);

	d->objectTreeView->expand(d->objectTreeModel->indexFromItem(d->rootMaterialNode));
	d->objectTreeView->scrollTo(d->objectTreeModel->indexFromItem(d->rootMaterialNode));

	// Стиль дерева под VS (опционально)
	d->objectTreeView->setStyleSheet(R"(
        QTreeView {
            background: #1E1E1E;
            color: #D4D4D4;
            gridline-color: #3E3E42;
            border: none;
        }
        QTreeView::item {
            padding: 2px 2px;
        }
        QTreeView::item:selected {
            background: #094771;
            color: white;
        }
        QHeaderView::section {
            background: #2D2D30;
            color: #CCCCCC;
            padding: 2px;
            border: none;
        }
    )");

	d->objectsSplitter->addWidget(d->objectTreeView);

	d->propertiesWidget = new PropertiesListWidget();
	d->propertiesWidget->setMinimumHeight(150);
	d->propertiesWidget->setStyleSheet("background: #252526;"); // Тёмный фон свойств
	d->objectsSplitter->addWidget(d->propertiesWidget);

	d->objectsSplitter->setStretchFactor(0, 1);
	d->objectsSplitter->setStretchFactor(1, 0);
	d->objectsSplitter->setSizes({ height() - 200, 200 });
	d->objectsSplitter->setHandleWidth(1);
	d->objectsSplitter->setStyleSheet("QSplitter::handle { background: #3E3E42; }");

	objectsLayout->addWidget(d->objectsSplitter);

	connect(d->objectTreeView, &QTreeView::activated, this, &MaterialObjects::onItemDoubleClicked);
	connect(d->objectTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MaterialObjects::onItemSelected);
	connect(d->propertiesWidget, &PropertiesListWidget::propertyChanged, this, &MaterialObjects::onPropertyChanged);
	connect(d->propertiesWidget, &PropertiesListWidget::propertyButtonClick, this, &MaterialObjects::onPropertyButtonClick);
	connect(d->objectTreeView, &QTreeView::customContextMenuRequested, this, &MaterialObjects::onCustomContextMenuRequested);
}

void MaterialObjects::onAddSubButtonTriggered(MaterialObjectTypes type) {
	auto index = d->selection;
	if (!index.isValid()) {
		index = d->objectTreeModel->indexFromItem(d->rootMaterialNode);
	}

	auto item = d->objectTreeModel->itemFromIndex(index);
	if (!item) {
		return;
	}

	auto subItem = MaterialObjectNode::appendNode(item, "new_item", type);
	d->objectTreeView->expand(d->objectTreeModel->indexFromItem(subItem));
	d->objectTreeView->scrollTo(d->objectTreeModel->indexFromItem(subItem));
}

void MaterialObjects::onItemSelected(const QModelIndex& current, const QModelIndex& previous) {
	d->selection = current;
	updateProperties();
}

void MaterialObjects::onItemDoubleClicked(const QModelIndex& index) {

}

void MaterialObjects::onCustomContextMenuRequested(const QPoint& pos) {
	const auto index = d->objectTreeView->indexAt(pos);
	if (!index.isValid()) {
		return;
	}

	auto item = d->objectTreeModel->itemFromIndex(index);
	if (!item) {
		return;
	}

	MaterialObjectNode node(item);
	MaterialObjectMenuActionsBuilder builder(node.type(), d->objectTreeView);
	const auto menu = builder.buildAddMenu();
	if (menu && !menu->actions().isEmpty()) {
		MaterialObjectMenuActionsBuilder::attachMenuAction(*menu, "action_add_directory", [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::Directory); });
		MaterialObjectMenuActionsBuilder::attachMenuAction(*menu, "action_add_materail", [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::Material); });
		MaterialObjectMenuActionsBuilder::attachMenuAction(*menu, "action_add_texture", [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::Texture); });
		MaterialObjectMenuActionsBuilder::attachMenuAction(*menu, "action_add_frag_shader", [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::FragmentShader); });
		MaterialObjectMenuActionsBuilder::attachMenuAction(*menu, "action_add_vert_shader", [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::VertexShader); });
		MaterialObjectMenuActionsBuilder::attachMenuAction(*menu, "action_add_albedo", [this]() { emit onAddSubButtonTriggered(MaterialObjectTypes::BaseColor); });

		menu->exec(d->objectTreeView->viewport()->mapToGlobal(pos));
	}
}

void MaterialObjects::updateProperties() {
	auto index = d->selection;
	if (!index.isValid()) {
		index = d->objectTreeModel->indexFromItem(d->rootMaterialNode);
	}

	auto item = d->objectTreeModel->itemFromIndex(index);
	if (!item) {
		d->propertiesWidget->clearProperties();
		return;
	}

	MaterialObjectNode node(item);
	const auto type = node.type();

	if (type == MaterialObjectTypes::Undefined ||
		type == MaterialObjectTypes::Directory) {
		d->propertiesWidget->clearProperties();
		return;
	}

	QList<PropertyData> props;

	props.append({
			"guid", "GUID", PropertyType::Text,
			QVariant(node.guid()), QVariant(), true, QStringList(),
			"", "Идентификатор (нельзя изменить)"
		});

	QString fileFilter;
	switch (type) {
		case MaterialObjectTypes::VertexShader:
			fileFilter = "Vertex Shader (*.vert)";
		case MaterialObjectTypes::FragmentShader:
			fileFilter = "Fragment Shader (*.frag)";
		{
			props.append({
					"path", "Путь к файлу", PropertyType::PathFile,
					node.path(), QVariant(), false, QStringList(),
					fileFilter, ""
				});
		}
	}

	switch (type) {
		case MaterialObjectTypes::VertexShader:
		case MaterialObjectTypes::FragmentShader: {
			props.append({
					"edit", "Редактировать", PropertyType::Button,
					QVariant(), QVariant(), false, QStringList(),
					QString(),
					"🖼️ Редактировать шейдер"
				});
		}
	}

	if (type == MaterialObjectTypes::BaseColor) {
		props.append({
				"color", "Базовый цвет", PropertyType::Color,
				QColor("#3A7BD5"), QVariant(), false
			});
	}

	d->propertiesWidget->setProperties(props);
}

void MaterialObjects::onPropertyChanged(const QString& propertyId, const QVariant& newValue) {
	auto item = d->objectTreeModel->itemFromIndex(d->selection);
	if (!item) {
		return;
	}

	MaterialObjectNode node(item);

	if (propertyId == "path") {
		node.setPath(newValue.toString());
	}
}

void MaterialObjects::onPropertyButtonClick(const QString& propertyId) {
	auto item = d->objectTreeModel->itemFromIndex(d->selection);
	if (!item) {
		return;
	}

	MaterialObjectNode node(item);

	if (propertyId == "edit") {
		emit editMaterialFile(node.type(), node.path());
		return;
	}
}
