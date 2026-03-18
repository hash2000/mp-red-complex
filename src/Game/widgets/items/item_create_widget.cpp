#include "Game/widgets/items/item_create_widget.h"
#include "Game/styles/items_styles.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/items_placement_service.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QSpinBox>
#include <QPainter>
#include <QDialog>
#include <QListWidget>
#include <QMessageBox>

class ItemCreateWidget::Private {
public:
	Private(ItemCreateWidget* parent)
		: q(parent) {
	}

	ItemCreateWidget* q;
	ItemEntity entity;
	ItemsService* itemsService;
	InventoriesService* inventoriesService;

	QLabel* imageLabel = nullptr;
	QTableWidget* paramsTable = nullptr;
	QPushButton* createButton = nullptr;
	QSpinBox* countSpinBox = nullptr;

	// Элементы выбора инвентаря
	QLabel* inventoryLabel = nullptr;
	QPushButton* selectInventoryButton = nullptr;

	// Данные об инвентарях
	QStringList availableInventoryIds;
	QString selectedInventoryId;
};

ItemCreateWidget::ItemCreateWidget(
	const ItemEntity& entity,
	ItemsService* itemsService,
	InventoriesService* inventoriesService,
	QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {
	d->entity = entity;
	d->itemsService = itemsService;
	d->inventoriesService = inventoriesService;

	setWindowTitle("Создание предмета");
	setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
	setWindowModality(Qt::WindowModal);

	setupLayout();
	updateItemImage();
	populateParamsTable();
	updateInventoryDisplay();

	// Подключаем кнопку создания
	connect(d->createButton, &QPushButton::clicked, this, [this]() {
		// Проверяем, выбран ли инвентарь
		if (d->selectedInventoryId.isEmpty()) {
			showInventorySelectionDialog();
			if (d->selectedInventoryId.isEmpty()) {
				// Пользователь не выбрал инвентарь (отменил диалог или нет доступных)
				return;
			}
		}

		const int count = d->countSpinBox->value();
		emit itemCreated(d->entity.id, count, d->selectedInventoryId);
		close();
		});

	// Подключаем кнопку выбора инвентаря
	if (d->selectInventoryButton) {
		connect(d->selectInventoryButton, &QPushButton::clicked,
			this, &ItemCreateWidget::onInventorySelectionRequested);
	}
}

ItemCreateWidget::~ItemCreateWidget() = default;

void ItemCreateWidget::setupLayout() {
	auto mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(20, 20, 20, 20);
	mainLayout->setSpacing(24);

	// === Левая часть: изображение предмета ===
	auto leftLayout = new QVBoxLayout();
	leftLayout->setSpacing(12);

	// Вычисляем размер изображения на основе размеров предмета
	const int imageWidth = d->entity.width * ItemsStyles::CELL_SIZE;
	const int imageHeight = d->entity.height * ItemsStyles::CELL_SIZE;

	d->imageLabel = new QLabel();
	d->imageLabel->setFixedSize(imageWidth, imageHeight);
	d->imageLabel->setAlignment(Qt::AlignCenter);
	d->imageLabel->setStyleSheet(R"(
		QLabel {
			background-color: rgba(30, 41, 59, 200);
			border: 2px dashed #4a5568;
			border-radius: 6px;
		}
	)");

	leftLayout->addWidget(d->imageLabel);
	leftLayout->addStretch();

	mainLayout->addLayout(leftLayout);

	// === Правая часть: таблица параметров ===
	auto rightLayout = new QVBoxLayout();
	rightLayout->setSpacing(16);

	// Заголовок
	auto titleLabel = new QLabel(d->entity.name);
	titleLabel->setStyleSheet(
		QString("color: %1; font-weight: 700; font-size: 16px;")
			.arg(ItemsStyles::rarityColor(d->entity.rarity)));
	rightLayout->addWidget(titleLabel);

	// Таблица параметров
	d->paramsTable = new QTableWidget();
	d->paramsTable->setColumnCount(2);
	d->paramsTable->horizontalHeader()->hide();
	d->paramsTable->verticalHeader()->hide();
	d->paramsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	d->paramsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	d->paramsTable->setShowGrid(false);
	d->paramsTable->setAlternatingRowColors(true);
	d->paramsTable->setStyleSheet(R"(
		QTableWidget {
			background-color: rgba(15, 23, 42, 200);
			border: 1px solid #4a5568;
			border-radius: 6px;
			gridline-color: #1e293b;
		}
		QTableWidget::item {
			padding: 6px;
			color: #e2e8f0;
		}
		QTableWidget::item:selected {
			background-color: #1e40af;
		}
	)");

	rightLayout->addWidget(d->paramsTable);
	rightLayout->addStretch();

	// Выбор количества
	auto countRow = new QHBoxLayout();
	countRow->addWidget(new QLabel("Количество:"));
	
	d->countSpinBox = new QSpinBox();
	d->countSpinBox->setRange(1, d->entity.maxStack);
	d->countSpinBox->setValue(1);
	d->countSpinBox->setFixedWidth(80);
	d->countSpinBox->setStyleSheet(R"(
		QSpinBox {
			background-color: rgba(30, 41, 59, 200);
			border: 1px solid #4a5568;
			border-radius: 4px;
			color: #e2e8f0;
			padding: 4px 8px;
		}
		QSpinBox::up-button, QSpinBox::down-button {
			border: none;
			background-color: #4a5568;
			width: 16px;
		}
		QSpinBox::up-button:hover, QSpinBox::down-button:hover {
			background-color: #64748b;
		}
	)");
	countRow->addStretch();
	countRow->addWidget(d->countSpinBox);
	rightLayout->addLayout(countRow);

	// Кнопка "Создать"
	d->createButton = new QPushButton("Создать предмет");
	d->createButton->setFixedHeight(36);
	d->createButton->setStyleSheet(R"(
		QPushButton {
			background-color: #10b981;
			color: #0f172a;
			border: none;
			border-radius: 6px;
			font-weight: 600;
			font-size: 13px;
		}
		QPushButton:hover {
			background-color: #0da271;
		}
		QPushButton:pressed {
			background-color: #0a8c62;
		}
	)");

	rightLayout->addWidget(d->createButton);

	// Блок выбора инвентаря
	auto inventoryFrame = new QWidget();
	inventoryFrame->setStyleSheet(R"(
		QWidget {
			background-color: rgba(30, 41, 59, 200);
			border: 1px solid #4a5568;
			border-radius: 6px;
			padding: 8px;
		}
	)");
	auto inventoryLayout = new QVBoxLayout(inventoryFrame);
	inventoryLayout->setContentsMargins(8, 8, 8, 8);
	inventoryLayout->setSpacing(4);

	// Заголовок
	auto inventoryTitleLabel = new QLabel("Инвентарь для создания:");
	inventoryTitleLabel->setStyleSheet("color: #94a3b8; font-weight: 600; font-size: 11px;");
	inventoryLayout->addWidget(inventoryTitleLabel);

	// Метка с выбранным инвентарём и кнопка выбора
	auto inventoryRow = new QHBoxLayout();
	
	d->inventoryLabel = new QLabel("Не выбрано");
	d->inventoryLabel->setStyleSheet("color: #e2e8f0; font-size: 12px;");
	d->inventoryLabel->setWordWrap(true);
	inventoryRow->addWidget(d->inventoryLabel, 1);

	d->selectInventoryButton = new QPushButton("Выбрать");
	d->selectInventoryButton->setFixedHeight(28);
	d->selectInventoryButton->setStyleSheet(R"(
		QPushButton {
			background-color: #3b82f6;
			color: #0f172a;
			border: none;
			border-radius: 4px;
			font-weight: 600;
			font-size: 11px;
			padding: 4px 12px;
		}
		QPushButton:hover {
			background-color: #2563eb;
		}
		QPushButton:pressed {
			background-color: #1d4ed8;
		}
	)");
	inventoryRow->addWidget(d->selectInventoryButton);

	inventoryLayout->addLayout(inventoryRow);
	rightLayout->addWidget(inventoryFrame);

	mainLayout->addLayout(rightLayout);

	// Устанавливаем фиксированный размер окна
	const int windowWidth = 500;
	const int windowHeight = 420;
	setFixedSize(windowWidth, windowHeight);
}

void ItemCreateWidget::updateItemImage() {
	if (!d->entity.icon.isNull()) {
		const int imageWidth = d->entity.width * ItemsStyles::CELL_SIZE;
		const int imageHeight = d->entity.height * ItemsStyles::CELL_SIZE;
		d->imageLabel->setPixmap(
			d->entity.icon.scaled(imageWidth, imageHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
	else {
		// Заглушка для отсутствующей иконки
		const int imageWidth = d->entity.width * ItemsStyles::CELL_SIZE;
		const int imageHeight = d->entity.height * ItemsStyles::CELL_SIZE;
		QPixmap placeholder(imageWidth, imageHeight);
		placeholder.fill(QColor("#1e293b"));
		QPainter painter(&placeholder);
		painter.setPen(QColor("#4a5568"));
		painter.drawText(placeholder.rect(), Qt::AlignCenter, d->entity.name.left(2));
		d->imageLabel->setPixmap(placeholder);
	}
}

void ItemCreateWidget::populateParamsTable() {
	auto table = d->paramsTable;
	int row = 0;

	// Вспомогательная функция для добавления строки
	auto addRow = [table, &row](const QString& key, const QString& value) {
		table->insertRow(row);
		auto keyLabel = new QLabel(key);
		keyLabel->setStyleSheet("color: #94a3b8; font-weight: 600; padding: 4px;");
		auto valueLabel = new QLabel(value);
		valueLabel->setStyleSheet("color: #e2e8f0; padding: 4px;");
		table->setCellWidget(row, 0, keyLabel);
		table->setCellWidget(row, 1, valueLabel);
		row++;
	};

	// Описание
	if (!d->entity.description.isEmpty()) {
		addRow("Описание", d->entity.description);
	}

	// Размеры
	addRow("Размер", QString("%1 x %2").arg(d->entity.width).arg(d->entity.height));

	// Максимум в стеке
	addRow("Макс. в стеке", QString::number(d->entity.maxStack));

	// Тип предмета
	QString typeText;
	switch (d->entity.type) {
	case ItemType::Equipment: typeText = "Экипировка"; break;
	case ItemType::Resource: typeText = "Ресурс"; break;
	case ItemType::Component: typeText = "Компонент"; break;
	case ItemType::Container: typeText = "Контейнер"; break;
	case ItemType::Recipe: typeText = "Рецепт"; break;
	}
	addRow("Тип", typeText);

	// Тип экипировки (если применимо)
	if (d->entity.equipmentType != ItemEquipmentType::None) {
		QString equipmentTypeText;
		switch (d->entity.equipmentType) {
		case ItemEquipmentType::Head: equipmentTypeText = "Голова"; break;
		case ItemEquipmentType::Body: equipmentTypeText = "Тело"; break;
		case ItemEquipmentType::Weapon: equipmentTypeText = "Оружие"; break;
		case ItemEquipmentType::Shield: equipmentTypeText = "Щит"; break;
		case ItemEquipmentType::Gloves: equipmentTypeText = "Перчатки"; break;
		case ItemEquipmentType::Boots: equipmentTypeText = "Обувь"; break;
		case ItemEquipmentType::Ring: equipmentTypeText = "Кольцо"; break;
		case ItemEquipmentType::Amulet: equipmentTypeText = "Амулет"; break;
		case ItemEquipmentType::Backpack: equipmentTypeText = "Рюкзак"; break;
		case ItemEquipmentType::Bag: equipmentTypeText = "Сумка"; break;
		default: equipmentTypeText = "—"; break;
		}
		addRow("Слот экипировки", equipmentTypeText);
	}

	// Редкость
	QString rarityText;
	switch (d->entity.rarity) {
	case ItemRarityType::Common: rarityText = "Обычное"; break;
	case ItemRarityType::Improved: rarityText = "Улучшенное"; break;
	case ItemRarityType::Stable: rarityText = "Качественное"; break;
	case ItemRarityType::Rare: rarityText = "Редкое"; break;
	case ItemRarityType::Unique: rarityText = "Уникальное"; break;
	}
	addRow("Редкость", rarityText);
}

void ItemCreateWidget::setAvailableInventories(const QStringList& inventoryIds, const QString& preselectedInventoryId) {
	d->availableInventoryIds = inventoryIds;
	
	// Если указан предварительный выбор и он есть в списке
	if (!preselectedInventoryId.isEmpty() && inventoryIds.contains(preselectedInventoryId)) {
		d->selectedInventoryId = preselectedInventoryId;
	}
	// Если только один инвентарь в списке, выбираем его автоматически
	else if (inventoryIds.size() == 1) {
		d->selectedInventoryId = inventoryIds.first();
	}
	
	updateInventoryDisplay();
}

QString ItemCreateWidget::selectedInventoryId() const {
	return d->selectedInventoryId;
}

void ItemCreateWidget::onInventorySelectionRequested() {
	showInventorySelectionDialog();
}

void ItemCreateWidget::onItemSelected() {
	// Этот слот может использоваться для дополнительных действий после выбора
	updateInventoryDisplay();
}

void ItemCreateWidget::updateInventoryDisplay() {
	if (!d->inventoryLabel) {
		return;
	}

	if (d->selectedInventoryId.isEmpty()) {
		d->inventoryLabel->setText("Не выбрано");
		d->inventoryLabel->setStyleSheet("color: #f59e0b; font-size: 12px; font-weight: 600;");
	}
	else {
		QString displayName = getInventoryDisplayName(d->selectedInventoryId);
		d->inventoryLabel->setText(displayName);
		d->inventoryLabel->setStyleSheet("color: #10b981; font-size: 12px; font-weight: 600;");
	}
}

void ItemCreateWidget::showInventorySelectionDialog() {
	// Проверяем наличие доступных инвентарей
	if (d->availableInventoryIds.isEmpty()) {
		QMessageBox::warning(this, "Нет доступных инвентарей",
			"Откройте хотя бы один инвентарь для создания предметов.");
		return;
	}

	// Если только один инвентарь, выбираем его автоматически
	if (d->availableInventoryIds.size() == 1) {
		d->selectedInventoryId = d->availableInventoryIds.first();
		updateInventoryDisplay();
		return;
	}

	// Создаём диалог выбора инвентаря
	QDialog dialog(this);
	dialog.setWindowTitle("Выберите инвентарь");
	dialog.setWindowModality(Qt::WindowModal);
	dialog.setMinimumSize(300, 200);

	auto layout = new QVBoxLayout(&dialog);

	auto listWidget = new QListWidget();
	listWidget->setStyleSheet(R"(
		QListWidget {
			background-color: rgba(15, 23, 42, 255);
			border: 1px solid #4a5568;
			border-radius: 6px;
			color: #e2e8f0;
		}
		QListWidget::item {
			padding: 8px;
		}
		QListWidget::item:selected {
			background-color: #1e40af;
		}
		QListWidget::item:hover {
			background-color: #1e3a8a;
		}
	)");

	// Заполняем список инвентарей
	for (const auto& inventoryId : d->availableInventoryIds) {
		QString displayName = getInventoryDisplayName(inventoryId);
		auto item = new QListWidgetItem(displayName);
		item->setData(Qt::UserRole, inventoryId);
		
		// Если этот инвентарь уже выбран, помечаем его
		if (inventoryId == d->selectedInventoryId) {
			item->setSelected(true);
		}
		
		listWidget->addItem(item);
	}

	layout->addWidget(listWidget);

	// Кнопки подтверждения и отмены
	auto buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();

	auto okButton = new QPushButton("OK");
	okButton->setFixedWidth(80);
	okButton->setStyleSheet(R"(
		QPushButton {
			background-color: #10b981;
			color: #0f172a;
			border: none;
			border-radius: 4px;
			font-weight: 600;
		}
		QPushButton:hover {
			background-color: #0da271;
		}
	)");

	auto cancelButton = new QPushButton("Отмена");
	cancelButton->setFixedWidth(80);
	cancelButton->setStyleSheet(R"(
		QPushButton {
			background-color: #64748b;
			color: #0f172a;
			border: none;
			border-radius: 4px;
			font-weight: 600;
		}
		QPushButton:hover {
			background-color: #475569;
		}
	)");

	buttonLayout->addWidget(okButton);
	buttonLayout->addWidget(cancelButton);
	layout->addLayout(buttonLayout);

	// Подключаем кнопки
	connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
	connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

	// Двойной клик по элементу списка
	connect(listWidget, &QListWidget::itemDoubleClicked, &dialog, [&dialog](QListWidgetItem* item) {
		Q_UNUSED(item);
		dialog.accept();
	});

	if (dialog.exec() == QDialog::Accepted && listWidget->currentItem()) {
		d->selectedInventoryId = listWidget->currentItem()->data(Qt::UserRole).toString();
		updateInventoryDisplay();
		emit onItemSelected();
	}
}

QString ItemCreateWidget::getInventoryDisplayName(const QString& inventoryId) const {
	// Пытаемся получить имя инвентаря через сервис
	if (d->inventoriesService) {
		// Получаем сервис размещения для этого инвентаря
		auto placementService = d->inventoriesService->placementService(
			QUuid::fromString(inventoryId), false);
		
		if (placementService) {
			// Можно попробовать получить дополнительную информацию об инвентаре
			// Пока просто возвращаем ID в более читаемом формате
			return placementService->placementName();
		}
	}

	// Если не удалось получить имя, возвращаем сокращённый ID
	return QString("Инвентарь %1").arg(inventoryId.left(8));
}
