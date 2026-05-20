#include "Game/widgets/materials/material_objects.h"
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
	QSplitter* objectsSplitter = nullptr;

	// Вкладка "Объекты"
	QTreeView* objectTreeView = nullptr;
	QStandardItemModel* objectTreeModel = nullptr;
	PropertiesListWidget* propertiesWidget = nullptr;
	QModelIndex selection;
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
	d->objectTreeModel->setHorizontalHeaderLabels({ "Name", "Type" });

	// Стиль дерева под VS (опционально)
	d->objectTreeView->setStyleSheet(R"(
        QTreeView {
            background: #1E1E1E;
            color: #D4D4D4;
            gridline-color: #3E3E42;
            border: none;
        }
        QTreeView::item {
            padding: 2px 4px;
        }
        QTreeView::item:selected {
            background: #094771;
            color: white;
        }
        QHeaderView::section {
            background: #2D2D30;
            color: #CCCCCC;
            padding: 4px;
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
}

void MaterialObjects::onAddSubButtonTriggered(MaterialObjectTypes type) {
	auto item = d->objectTreeModel->itemFromIndex(d->selection);
	if (!item) {
		item = d->objectTreeModel->invisibleRootItem();
	}

	MaterialObjectNode::appendNode(item, "new_item", type);
}

void MaterialObjects::onItemSelected(const QModelIndex& current, const QModelIndex& previous) {
	d->selection = current;
	updateProperties();
}

void MaterialObjects::onItemDoubleClicked(const QModelIndex& index) {

}

void MaterialObjects::updateProperties() {
	auto item = d->objectTreeModel->itemFromIndex(d->selection);
	if (!item) {
		d->propertiesWidget->clearProperties();
		return;
	}

	MaterialObjectNode node(item);
	const auto type = node.type();

	if (type == MaterialObjectTypes::Directory) {
		d->propertiesWidget->clearProperties();
		return;
	}

	QList<PropertyData> props;

	props.append({
			"guid", "GUID", PropertyType::Text,
			QVariant(node.guid()), QVariant(), true, QStringList(),
			"", "Идентификатор (нельзя изменить)"
		});

	switch (type) {
		case MaterialObjectTypes::VertexShader:
		case MaterialObjectTypes::FragmentShader: {
			props.append({
					"path", "Путь к файлу", PropertyType::PathFile,
					node.path(), QVariant(), false, QStringList(),
					type == MaterialObjectTypes::VertexShader ?
						"Vertex Shader (*.vert)" :
						"Fragment Shader (*.frag)"
				, ""
				});
			props.append({
					"edit", "Редактировать", PropertyType::Button,
					QVariant(), QVariant(), false, QStringList(),
					QString(), 
					"🖼️ Редактировать шейдер"
				});
		}
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


}
