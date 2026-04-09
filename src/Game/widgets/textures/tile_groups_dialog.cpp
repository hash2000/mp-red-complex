#include "Game/widgets/textures/tile_groups_dialog.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include "DataLayer/textures/i_tile_groups_data_provider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class TileGroupsDialog::Private {
public:
	Private(TileGroupsDialog* parent) : q(parent) {}
	TileGroupsDialog* q;

	TilesService* tilesService = nullptr;
	QString texturePath;

	// Элементы UI
	QListWidget* groupsList = nullptr;
	QLineEdit* nameEdit = nullptr;
	QPushButton* renameButton = nullptr;
	QPushButton* selectButton = nullptr;
	QPushButton* deleteButton = nullptr;

	// Выбранная группа
	std::optional<TileGroup> resultGroup;

	// Полный кэш групп для быстрого доступа
	QList<TileGroup> cachedGroups;
};

TileGroupsDialog::TileGroupsDialog(
	TilesService* tilesService,
	const QString& texturePath,
	QWidget* parent)
	: QDialog(parent)
	, d(std::make_unique<Private>(this)) {
	d->tilesService = tilesService;
	d->texturePath = texturePath;

	setWindowTitle("Группы тайлов");
	setModal(true);
	setMinimumSize(400, 300);
	setStyleSheet(R"(
		QDialog {
			background-color: #1a202c;
		}
	)");

	setupLayout();
	refreshList();
}

TileGroupsDialog::~TileGroupsDialog() = default;

std::optional<TileGroup> TileGroupsDialog::selectedGroup() const {
	return d->resultGroup;
}

void TileGroupsDialog::setupLayout() {
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(12, 12, 12, 12);
	mainLayout->setSpacing(8);

	// Заголовок
	auto* titleLabel = new QLabel("Группы тайлов:");
	titleLabel->setStyleSheet("color: #e2e8f0; font-size: 13px; font-weight: bold; padding: 4px 0;");
	mainLayout->addWidget(titleLabel);

	// Список групп
	d->groupsList = new QListWidget();
	d->groupsList->setObjectName("GroupsList");
	d->groupsList->setStyleSheet(R"(
		#GroupsList {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 4px;
		}
		#GroupsList::item {
			padding: 6px;
			border-bottom: 1px solid #4a5568;
		}
		#GroupsList::item:selected {
			background-color: #4a5568;
		}
		#GroupsList::item:hover {
			background-color: #4a5568;
		}
	)");
	mainLayout->addWidget(d->groupsList, 1);

	// Поле ввода имени
	auto* nameLayout = new QHBoxLayout();
	auto* nameLabel = new QLabel("Имя:");
	nameLabel->setStyleSheet("color: #a0aec0; font-size: 11px;");
	nameLayout->addWidget(nameLabel);

	d->nameEdit = new QLineEdit();
	d->nameEdit->setPlaceholderText("Имя группы...");
	d->nameEdit->setStyleSheet(R"(
		QLineEdit {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 4px 8px;
			font-size: 12px;
		}
		QLineEdit:focus {
			border: 1px solid #63b3ed;
		}
	)");
	nameLayout->addWidget(d->nameEdit, 1);
	mainLayout->addLayout(nameLayout);

	// Кнопки
	auto* buttonsLayout = new QHBoxLayout();

	d->renameButton = new QPushButton("✏ Переименовать");
	d->renameButton->setEnabled(false);
	d->renameButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 6px 12px;
			font-size: 12px;
		}
		QPushButton:hover:enabled {
			background-color: #4a5568;
			border: 1px solid #718096;
		}
		QPushButton:pressed:enabled {
			background-color: #1a202c;
		}
		QPushButton:disabled {
			color: #718096;
		}
	)");

	d->selectButton = new QPushButton("✓ Выбрать");
	d->selectButton->setEnabled(false);
	d->selectButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 6px 12px;
			font-size: 12px;
		}
		QPushButton:hover:enabled {
			background-color: #48bb78;
			border: 1px solid #68d391;
		}
		QPushButton:pressed:enabled {
			background-color: #38a169;
		}
		QPushButton:disabled {
			color: #718096;
		}
	)");

	d->deleteButton = new QPushButton("🗑 Удалить");
	d->deleteButton->setEnabled(false);
	d->deleteButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 6px 12px;
			font-size: 12px;
		}
		QPushButton:hover:enabled {
			background-color: #f56565;
			border: 1px solid #fc8181;
		}
		QPushButton:pressed:enabled {
			background-color: #e53e3e;
		}
		QPushButton:disabled {
			color: #718096;
		}
	)");

	buttonsLayout->addWidget(d->renameButton);
	buttonsLayout->addWidget(d->selectButton);
	buttonsLayout->addWidget(d->deleteButton);
	mainLayout->addLayout(buttonsLayout);

	// Сигналы
	connect(d->groupsList, &QListWidget::itemClicked,
	        this, &TileGroupsDialog::onListItemClicked);
	connect(d->groupsList, &QListWidget::itemDoubleClicked,
	        this, &TileGroupsDialog::onListItemDoubleClicked);
	connect(d->renameButton, &QPushButton::clicked,
	        this, &TileGroupsDialog::onRenameClicked);
	connect(d->selectButton, &QPushButton::clicked,
	        this, &TileGroupsDialog::onSelectClicked);
	connect(d->deleteButton, &QPushButton::clicked,
	        this, &TileGroupsDialog::onDeleteClicked);
}

void TileGroupsDialog::refreshList() {
	d->groupsList->clear();

	d->cachedGroups = d->tilesService->getGroups(d->texturePath);
	for (int i = 0; i < d->cachedGroups.size(); ++i) {
		const auto& group = d->cachedGroups[i];
		auto* item = new QListWidgetItem(QString("%1 (%2 тайлов)").arg(group.name).arg(group.tileIds.size()));
		item->setData(Qt::UserRole, i);  // Сохраняем индекс в кэше
		d->groupsList->addItem(item);
	}
}

void TileGroupsDialog::onListItemClicked(QListWidgetItem* item) {
	if (item) {
		const int index = item->data(Qt::UserRole).toInt();
		if (index >= 0 && index < d->cachedGroups.size()) {
			d->nameEdit->setText(d->cachedGroups[index].name);
			d->renameButton->setEnabled(true);
			d->selectButton->setEnabled(true);
			d->deleteButton->setEnabled(true);
		}
	}
}

void TileGroupsDialog::onListItemDoubleClicked(QListWidgetItem* item) {
	if (item) {
		const int index = item->data(Qt::UserRole).toInt();
		if (index >= 0 && index < d->cachedGroups.size()) {
			d->resultGroup = d->cachedGroups[index];
			accept();
		}
	}
}

void TileGroupsDialog::onRenameClicked() {
	auto* currentItem = d->groupsList->currentItem();
	if (!currentItem) {
		return;
	}

	const int index = currentItem->data(Qt::UserRole).toInt();
	if (index < 0 || index >= d->cachedGroups.size()) {
		return;
	}

	const QString newName = d->nameEdit->text().trimmed();
	if (newName.isEmpty()) {
		return;
	}

	// Обновляем группу
	TileGroup updatedGroup = d->cachedGroups[index];
	updatedGroup.name = newName;
	d->tilesService->updateGroup(d->texturePath, updatedGroup);

	refreshList();
}

void TileGroupsDialog::onSelectClicked() {
	auto* currentItem = d->groupsList->currentItem();
	if (!currentItem) {
		return;
	}

	const int index = currentItem->data(Qt::UserRole).toInt();
	if (index >= 0 && index < d->cachedGroups.size()) {
		d->resultGroup = d->cachedGroups[index];
		accept();
	}
}

void TileGroupsDialog::onDeleteClicked() {
	auto* currentItem = d->groupsList->currentItem();
	if (!currentItem) {
		return;
	}

	const int index = currentItem->data(Qt::UserRole).toInt();
	if (index < 0 || index >= d->cachedGroups.size()) {
		return;
	}

	d->tilesService->deleteGroup(d->cachedGroups[index].id);

	d->nameEdit->clear();
	d->renameButton->setEnabled(false);
	d->selectButton->setEnabled(false);
	d->deleteButton->setEnabled(false);

	refreshList();
}
