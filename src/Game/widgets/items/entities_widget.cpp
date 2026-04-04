#include "Game/widgets/items/entities_widget.h"
#include "Game/widgets/items/entty_entry_widget.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/i_items_placement_service.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QListWidget>
#include <QMessageBox>

class EntitiesWidget::Private {
public:
	Private(EntitiesWidget* parent)
		: q(parent) {
	}

	EntitiesWidget* q;
	QScrollArea* scrollArea;
	QWidget* contentWidget;
	QVBoxLayout* contentLayout;
	ItemsService* service;
	InventoriesService* inventoriesService;

	// Элементы выбора инвентаря
	QWidget* inventorySelectorWidget = nullptr;
	QLabel* inventoryLabel = nullptr;
	QPushButton* selectInventoryButton = nullptr;

	// Данные об инвентарях
	QStringList availableInventoryIds;
	QString selectedInventoryId;
};

EntitiesWidget::EntitiesWidget(
	ItemsService* service,
	InventoriesService* inventoriesService,
	QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {

	d->service = service;
	d->inventoriesService = inventoriesService;

	// Основной вертикальный лейаут
	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	// Виджет выбора инвентаря (вверху)
	d->inventorySelectorWidget = new QWidget();
	d->inventorySelectorWidget->setMinimumHeight(50);
	d->inventorySelectorWidget->setMaximumHeight(70);
	d->inventorySelectorWidget->setStyleSheet(R"(
		QWidget {
			background-color: rgba(30, 41, 59, 200);
			border-bottom: 1px solid #4a5568;
		}
	)");
	auto inventoryLayout = new QVBoxLayout(d->inventorySelectorWidget);
	inventoryLayout->setContentsMargins(10, 4, 10, 4);
	inventoryLayout->setSpacing(4);

	// Заголовок
	auto inventoryTitleLabel = new QLabel("Инвентарь для создания:");
	inventoryTitleLabel->setStyleSheet("color: #94a3b8; font-weight: 600; font-size: 11px;");
	inventoryLayout->addWidget(inventoryTitleLabel);

	// Метка с выбранным инвентарём и кнопка выбора
	auto inventoryRow = new QHBoxLayout();

	d->inventoryLabel = new QLabel("Не выбрано");
	d->inventoryLabel->setStyleSheet("color: #f59e0b; font-size: 12px;");
	d->inventoryLabel->setWordWrap(true);
	inventoryRow->addWidget(d->inventoryLabel, 1);

	d->selectInventoryButton = new QPushButton("Выбрать");
	d->selectInventoryButton->setFixedHeight(32);
	d->selectInventoryButton->setStyleSheet(R"(
		QPushButton {
			background-color: #3b82f6;
			color: #0f172a;
			border: none;
			border-radius: 4px;
			font-weight: 600;
			font-size: 12px;
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
	mainLayout->addWidget(d->inventorySelectorWidget);

	// Подключаем кнопку выбора инвентаря
	connect(d->selectInventoryButton, &QPushButton::clicked,
		this, &EntitiesWidget::onInventorySelectionRequested);

	// Область прокрутки
	d->scrollArea = new QScrollArea();
	d->scrollArea->setWidgetResizable(true);
	d->scrollArea->setFrameShape(QFrame::NoFrame);
	d->scrollArea->setStyleSheet(R"(
      QScrollArea {
          background-color: #0f172a;
          border: none;
      }
      QScrollBar:vertical {
          border: none;
          background: #1e293b;
          width: 12px;
          margin: 0px;
      }
      QScrollBar::handle:vertical {
          background: #4a5568;
          min-height: 30px;
          border-radius: 6px;
      }
      QScrollBar::handle:vertical:hover {
          background: #64748b;
      }
      QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
          height: 0px;
      }
  )");

	// Контент для прокрутки
	d->contentWidget = new QWidget();
	d->contentLayout = new QVBoxLayout(d->contentWidget);
	d->contentLayout->setContentsMargins(10, 10, 10, 10);
	d->contentLayout->setSpacing(8);
	d->contentLayout->addStretch(); // Для выравнивания элементов сверху

	d->contentWidget->setStyleSheet("background-color: transparent;");
	d->scrollArea->setWidget(d->contentWidget);

	mainLayout->addWidget(d->scrollArea);
}

EntitiesWidget::~EntitiesWidget() {
	clear();
}

void EntitiesWidget::addItemEntity(const ItemEntity& entity) {
	// Убираем stretch перед добавлением нового элемента
	if (d->contentLayout->count() > 0) {
		delete d->contentLayout->takeAt(d->contentLayout->count() - 1);
	}

	auto entry = new EntityEntryWidget(entity, this);
	d->contentLayout->addWidget(entry);

	// Возвращаем stretch в конец
	d->contentLayout->addStretch();

	// Подключаем сигнал записи к сигналу окна (передаём текущий выбранный инвентарь)
	connect(entry, &EntityEntryWidget::createRequested, this, [this](const QString& itemId) {
		// Если инвентарь не выбран, запрашиваем диалог выбора
		if (d->selectedInventoryId.isEmpty()) {
			emit inventorySelectionRequested();
			// После показа диалога, если инвентарь выбран, создаём предмет
			if (!d->selectedInventoryId.isEmpty()) {
				emit itemCreateRequested(itemId, d->selectedInventoryId);
			}
		}
		else {
			emit itemCreateRequested(itemId, d->selectedInventoryId);
		}
	});
}

void EntitiesWidget::clear() {
	QLayoutItem* item;
	while ((item = d->contentLayout->takeAt(0)) != nullptr) {
		if (item->widget()) {
			delete item->widget();
		}
		delete item;
	}

	d->contentLayout->addStretch();
}

void EntitiesWidget::setAvailableInventories(const QStringList& inventoryIds, const QString& preselectedInventoryId) {
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

QString EntitiesWidget::selectedInventoryId() const {
	return d->selectedInventoryId;
}

void EntitiesWidget::setInventoriesService(InventoriesService* service) {
	d->inventoriesService = service;
}

void EntitiesWidget::onInventorySelectionRequested() {
	// Запрашиваем у родителя (EntitiesWindow) обновление списка инвентарей
	emit inventorySelectionRequested();
	
	// Показываем диалог выбора после обновления списка
	showInventorySelectionDialog();
}

void EntitiesWidget::updateInventoryDisplay() {
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

void EntitiesWidget::showInventorySelectionDialog() {
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
		emit inventorySelectionChanged(d->selectedInventoryId);
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
		emit inventorySelectionChanged(d->selectedInventoryId);
	}
}

QString EntitiesWidget::getInventoryDisplayName(const QString& inventoryId) const {
	// Пытаемся получить имя инвентаря через сервис
	if (d->inventoriesService) {
		// Получаем сервис размещения для этого инвентаря
		auto placementService = d->inventoriesService->placementService(
			QUuid::fromString(inventoryId), false);

		if (placementService) {
			return placementService->placementName();
		}
	}

	// Если не удалось получить имя, возвращаем сокращённый ID
	return QString("Инвентарь %1").arg(inventoryId.left(8));
}
