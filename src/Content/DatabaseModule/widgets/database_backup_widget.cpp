#include "Content/DatabaseModule/widgets/database_backup_widget.h"
#include "Content/DatabaseModule/models/database_backup.h"
#include "Content/DatabaseModule/services/database_backup_service.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStyle>
#include <QTableWidget>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

class DatabaseBackupWidget::Private {
public:
	explicit Private(DatabaseBackupWidget* parent)
		: q(parent) {
	}

	/// Private не наследует QObject — переводы берём у самого виджета.
	static QString tr(const char* text) {
		return DatabaseBackupWidget::tr(text);
	}

	DatabaseBackupWidget* q = nullptr;
	DatabaseBackupService* service = nullptr;

	QTreeWidget* stores = nullptr;
	QLabel* titleLabel = nullptr;
	QLabel* pathLabel = nullptr;
	QLabel* metaLabel = nullptr;
	QTableWidget* backups = nullptr;
	QLabel* statusLabel = nullptr;
	QPushButton* createButton = nullptr;
	QPushButton* restoreButton = nullptr;
	QPushButton* deleteButton = nullptr;
	QPushButton* openFolderButton = nullptr;
	QPushButton* refreshButton = nullptr;

	QString lastNotifiedAlias;

	void buildUi();
	void reloadStores();
	void reloadBackups();
	void updateDetails();
	void updateButtons();
	void notifySelection();
	void setStatus(const QString& message, const QString& state);

	QString selectedAlias() const {
		const auto* item = stores ? stores->currentItem() : nullptr;
		return item ? item->data(0, Qt::UserRole).toString() : QString();
	}

	QString selectedBackupPath() const {
		if (!backups || backups->currentRow() < 0) {
			return {};
		}
		const auto* item = backups->item(backups->currentRow(), 0);
		return item ? item->data(Qt::UserRole).toString() : QString();
	}
};

void DatabaseBackupWidget::Private::setStatus(const QString& message, const QString& state) {
	if (!statusLabel) {
		return;
	}

	statusLabel->setText(message);
	statusLabel->setProperty("state", state);
	statusLabel->style()->unpolish(statusLabel);
	statusLabel->style()->polish(statusLabel);
	emit q->statusChanged(message, state == QStringLiteral("error"));
}

void DatabaseBackupWidget::Private::buildUi() {
	auto* root = new QVBoxLayout(q);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	// Панель живёт и в боковой зоне (узкой), и в документе (широком),
	// поэтому хранилища располагаются над копиями, а не слева от них.
	auto* splitter = new QSplitter(Qt::Vertical, q);
	splitter->setObjectName(QStringLiteral("DatabaseBackupSplitter"));
	splitter->setChildrenCollapsible(false);

	stores = new QTreeWidget();
	stores->setObjectName(QStringLiteral("DatabaseBackupStores"));
	stores->setColumnCount(2);
	stores->setHeaderLabels({ tr("Хранилище"), tr("Копий") });
	stores->setRootIsDecorated(false);
	stores->setUniformRowHeights(true);
	stores->setSelectionMode(QAbstractItemView::SingleSelection);
	stores->setSelectionBehavior(QAbstractItemView::SelectRows);
	stores->header()->setStretchLastSection(false);
	stores->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	stores->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	auto* details = new QWidget();
	details->setObjectName(QStringLiteral("DatabaseBackupDetails"));
	auto* detailsLayout = new QVBoxLayout(details);
	detailsLayout->setContentsMargins(0, 0, 0, 0);
	detailsLayout->setSpacing(6);

	titleLabel = new QLabel(tr("Хранилище не выбрано"), details);
	titleLabel->setObjectName(QStringLiteral("DatabaseBackupStoreTitle"));
	titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	detailsLayout->addWidget(titleLabel);

	pathLabel = new QLabel(details);
	pathLabel->setObjectName(QStringLiteral("DatabaseBackupStorePath"));
	pathLabel->setWordWrap(true);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	detailsLayout->addWidget(pathLabel);

	metaLabel = new QLabel(details);
	metaLabel->setObjectName(QStringLiteral("DatabaseBackupStoreMeta"));
	metaLabel->setWordWrap(true);
	detailsLayout->addWidget(metaLabel);

	backups = new QTableWidget(details);
	backups->setObjectName(QStringLiteral("DatabaseBackupList"));
	backups->setColumnCount(3);
	backups->setHorizontalHeaderLabels({ tr("Копия"), tr("Создана"), tr("Размер") });
	backups->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	backups->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	backups->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	backups->verticalHeader()->setVisible(false);
	backups->setSelectionBehavior(QAbstractItemView::SelectRows);
	backups->setSelectionMode(QAbstractItemView::SingleSelection);
	backups->setEditTriggers(QAbstractItemView::NoEditTriggers);
	backups->setShowGrid(false);
	detailsLayout->addWidget(backups, 1);

	// Две строки кнопок: панель может быть уже, чем суммарная ширина действий.
	// «Удалить» — разрушающее действие, поэтому оно отделено и прижато вправо.
	auto* primaryButtons = new QHBoxLayout();
	primaryButtons->setContentsMargins(0, 0, 0, 0);
	primaryButtons->setSpacing(6);

	createButton = new QPushButton(tr("Создать копию"), details);
	createButton->setObjectName(QStringLiteral("DatabaseBackupCreate"));
	createButton->setToolTip(tr("Сохранить копию файла базы в каталог резервных копий"));
	primaryButtons->addWidget(createButton);

	restoreButton = new QPushButton(tr("Восстановить"), details);
	restoreButton->setObjectName(QStringLiteral("DatabaseBackupRestore"));
	restoreButton->setToolTip(tr("Заменить базу выбранной копией"));
	primaryButtons->addWidget(restoreButton);

	primaryButtons->addStretch(1);

	deleteButton = new QPushButton(tr("Удалить"), details);
	deleteButton->setObjectName(QStringLiteral("DatabaseBackupDelete"));
	deleteButton->setToolTip(tr("Удалить выбранный файл копии"));
	primaryButtons->addWidget(deleteButton, 0, Qt::AlignRight);

	detailsLayout->addLayout(primaryButtons);

	auto* secondaryButtons = new QHBoxLayout();
	secondaryButtons->setContentsMargins(0, 0, 0, 0);
	secondaryButtons->setSpacing(6);

	openFolderButton = new QPushButton(tr("Каталог"), details);
	openFolderButton->setObjectName(QStringLiteral("DatabaseBackupOpenFolder"));
	openFolderButton->setToolTip(tr("Открыть каталог резервных копий хранилища"));
	secondaryButtons->addWidget(openFolderButton);

	refreshButton = new QPushButton(tr("Обновить"), details);
	refreshButton->setObjectName(QStringLiteral("DatabaseBackupRefresh"));
	refreshButton->setToolTip(tr("Перечитать список хранилищ и копий"));
	secondaryButtons->addWidget(refreshButton);

	secondaryButtons->addStretch(1);
	detailsLayout->addLayout(secondaryButtons);

	statusLabel = new QLabel(details);
	statusLabel->setObjectName(QStringLiteral("DatabaseBackupStatus"));
	statusLabel->setWordWrap(true);
	statusLabel->setProperty("state", QString());
	detailsLayout->addWidget(statusLabel);

	splitter->addWidget(stores);
	splitter->addWidget(details);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 3);
	root->addWidget(splitter, 1);

	QObject::connect(stores, &QTreeWidget::currentItemChanged, q,
		[this](QTreeWidgetItem*, QTreeWidgetItem*) {
			updateDetails();
			reloadBackups();
			notifySelection();
		});
	QObject::connect(backups, &QTableWidget::itemSelectionChanged, q, [this] {
		updateButtons();
	});
	QObject::connect(createButton, &QPushButton::clicked, q, &DatabaseBackupWidget::createBackup);
	QObject::connect(restoreButton, &QPushButton::clicked, q,
		&DatabaseBackupWidget::restoreSelectedBackup);
	QObject::connect(deleteButton, &QPushButton::clicked, q,
		&DatabaseBackupWidget::deleteSelectedBackup);
	QObject::connect(openFolderButton, &QPushButton::clicked, q,
		&DatabaseBackupWidget::openBackupFolder);
	QObject::connect(refreshButton, &QPushButton::clicked, q, &DatabaseBackupWidget::refresh);
}

void DatabaseBackupWidget::Private::updateDetails() {
	const auto alias = selectedAlias();
	if (alias.isEmpty() || !service) {
		titleLabel->setText(tr("Хранилище не выбрано"));
		pathLabel->clear();
		metaLabel->clear();
		updateButtons();
		return;
	}

	const auto info = service->store(alias);
	if (!info) {
		titleLabel->setText(alias);
		pathLabel->clear();
		metaLabel->clear();
		updateButtons();
		return;
	}

	titleLabel->setText(info->encrypted
		? tr("%1 · 🔒 зашифрована").arg(info->alias)
		: info->alias);
	pathLabel->setText(info->path);
	metaLabel->setText(tr("Файл: %1 · WAL: %2 мс / %3 МБ · копий: %4")
		.arg(info->fileExists ? QLocale().formattedDataSize(info->fileSize) : tr("не создан"))
		.arg(info->walCheckInterval)
		.arg(info->walMaxSizeMb)
		.arg(info->backupCount));
	updateButtons();
}

void DatabaseBackupWidget::Private::reloadBackups() {
	if (!backups) {
		return;
	}

	backups->clearContents();
	backups->setRowCount(0);

	const auto alias = selectedAlias();
	if (alias.isEmpty() || !service) {
		updateButtons();
		return;
	}

	const auto items = service->backups(alias);
	backups->setRowCount(items.size());
	for (int row = 0; row < items.size(); row++) {
		const auto& info = items.at(row);
		const auto fileName = QFileInfo(info.path).fileName();

		auto* nameItem = new QTableWidgetItem(info.automatic
			? tr("%1 (перед восстановлением)").arg(fileName)
			: fileName);
		nameItem->setData(Qt::UserRole, info.path);
		nameItem->setToolTip(info.path);
		backups->setItem(row, 0, nameItem);

		auto* createdAt = new QTableWidgetItem(QLocale().toString(info.createdAt, QLocale::ShortFormat));
		createdAt->setData(Qt::UserRole, info.path);
		createdAt->setToolTip(info.path);
		backups->setItem(row, 1, createdAt);

		auto* size = new QTableWidgetItem(QLocale().formattedDataSize(info.size));
		size->setData(Qt::UserRole, info.path);
		backups->setItem(row, 2, size);
	}

	if (backups->rowCount() > 0) {
		backups->selectRow(0);
	}
	updateButtons();
}

void DatabaseBackupWidget::Private::updateButtons() {
	const bool hasStore = !selectedAlias().isEmpty();
	const bool hasBackup = !selectedBackupPath().isEmpty();
	const bool available = service != nullptr;

	if (createButton) {
		createButton->setEnabled(available && hasStore);
	}
	if (restoreButton) {
		restoreButton->setEnabled(available && hasStore && hasBackup);
	}
	if (deleteButton) {
		deleteButton->setEnabled(available && hasStore && hasBackup);
	}
	if (openFolderButton) {
		openFolderButton->setEnabled(available && hasStore);
	}
	if (refreshButton) {
		refreshButton->setEnabled(available);
	}
}

void DatabaseBackupWidget::Private::notifySelection() {
	const auto alias = selectedAlias();
	if (alias == lastNotifiedAlias) {
		return;
	}

	lastNotifiedAlias = alias;
	if (!alias.isEmpty()) {
		emit q->storeSelected(alias);
	}
}

void DatabaseBackupWidget::Private::reloadStores() {
	if (!service) {
		setStatus(tr("Сервис резервного копирования недоступен"), QStringLiteral("error"));
		return;
	}

	const auto previous = selectedAlias();
	const auto items = service->stores();

	{
		const QSignalBlocker blocker(stores);
		stores->clear();
		for (const auto& store : items) {
			auto* item = new QTreeWidgetItem(stores);
			item->setData(0, Qt::UserRole, store.alias);
			item->setText(0, store.encrypted ? tr("%1 🔒").arg(store.alias) : store.alias);
			item->setText(1, QString::number(store.backupCount));
			item->setToolTip(0, store.path);
			if (!store.fileExists) {
				item->setToolTip(0, tr("%1\nФайл базы ещё не создан").arg(store.path));
			}
		}

		auto* selected = static_cast<QTreeWidgetItem*>(nullptr);
		for (int row = 0; row < stores->topLevelItemCount(); row++) {
			auto* item = stores->topLevelItem(row);
			if (item->data(0, Qt::UserRole).toString() == previous) {
				selected = item;
				break;
			}
		}
		if (!selected && stores->topLevelItemCount() > 0) {
			selected = stores->topLevelItem(0);
		}
		if (selected) {
			stores->setCurrentItem(selected);
		}
	}

	updateDetails();
	reloadBackups();
	notifySelection();

	if (items.isEmpty()) {
		setStatus(tr("Хранилища не найдены: добавьте их в bootstrap.yaml "
			"(stores.items) или в контекст .store"), QString());
	}
}

DatabaseBackupWidget::DatabaseBackupWidget(DatabaseBackupService* service, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->service = service;
	setObjectName(QStringLiteral("DatabaseBackupWidget"));
	d->buildUi();

	if (d->service) {
		connect(d->service, &DatabaseBackupService::backupsChanged, this,
			[this](const QString&) { refresh(); });
	}

	d->setStatus(tr("Выберите хранилище и создайте резервную копию"), QString());
	d->reloadStores();
}

DatabaseBackupWidget::~DatabaseBackupWidget() = default;

DatabaseBackupService* DatabaseBackupWidget::service() const {
	return d->service;
}

QString DatabaseBackupWidget::selectedStore() const {
	return d->selectedAlias();
}

bool DatabaseBackupWidget::selectStore(const QString& alias) {
	if (!d->stores || alias.isEmpty()) {
		return false;
	}

	for (int row = 0; row < d->stores->topLevelItemCount(); row++) {
		auto* item = d->stores->topLevelItem(row);
		if (item->data(0, Qt::UserRole).toString() == alias) {
			d->stores->setCurrentItem(item);
			return true;
		}
	}

	return false;
}

void DatabaseBackupWidget::refresh() {
	d->reloadStores();
}

void DatabaseBackupWidget::createBackup() {
	const auto alias = d->selectedAlias();
	if (alias.isEmpty() || !d->service) {
		d->setStatus(tr("Сначала выберите хранилище"), QStringLiteral("error"));
		return;
	}

	QString error;
	const auto created = d->service->createBackup(alias, &error);
	if (!created) {
		d->setStatus(error, QStringLiteral("error"));
		emit errorOccurred(error);
		return;
	}

	d->setStatus(tr("Создана копия «%1» (%2)")
		.arg(QFileInfo(created->path).fileName(), QLocale().formattedDataSize(created->size)),
		QStringLiteral("success"));
	emit backupCreated(alias, created->path);
	refresh();
}

void DatabaseBackupWidget::restoreSelectedBackup() {
	const auto alias = d->selectedAlias();
	const auto backupPath = d->selectedBackupPath();
	if (alias.isEmpty() || backupPath.isEmpty() || !d->service) {
		d->setStatus(tr("Выберите копию для восстановления"), QStringLiteral("error"));
		return;
	}

	const auto answer = QMessageBox::warning(this, tr("Восстановление базы данных"),
		tr("Текущая база «%1» будет заменена копией:\n%2\n\n"
			"Перед заменой автоматически создаётся страховочная копия. Продолжить?")
			.arg(alias, QFileInfo(backupPath).fileName()),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (answer != QMessageBox::Yes) {
		return;
	}

	QString error;
	if (!d->service->restoreBackup(alias, backupPath, &error)) {
		d->setStatus(error, QStringLiteral("error"));
		emit errorOccurred(error);
		return;
	}

	d->setStatus(tr("База «%1» восстановлена из копии «%2»")
		.arg(alias, QFileInfo(backupPath).fileName()), QStringLiteral("success"));
	emit databaseRestored(alias, backupPath);
	refresh();
}

void DatabaseBackupWidget::deleteSelectedBackup() {
	const auto alias = d->selectedAlias();
	const auto backupPath = d->selectedBackupPath();
	if (alias.isEmpty() || backupPath.isEmpty() || !d->service) {
		d->setStatus(tr("Выберите копию для удаления"), QStringLiteral("error"));
		return;
	}

	const auto answer = QMessageBox::question(this, tr("Удаление копии"),
		tr("Удалить файл копии «%1»?").arg(QFileInfo(backupPath).fileName()),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (answer != QMessageBox::Yes) {
		return;
	}

	QString error;
	if (!d->service->removeBackup(alias, backupPath, &error)) {
		d->setStatus(error, QStringLiteral("error"));
		emit errorOccurred(error);
		return;
	}

	d->setStatus(tr("Копия «%1» удалена").arg(QFileInfo(backupPath).fileName()),
		QStringLiteral("success"));
	refresh();
}

void DatabaseBackupWidget::openBackupFolder() {
	const auto alias = d->selectedAlias();
	if (alias.isEmpty() || !d->service) {
		d->setStatus(tr("Сначала выберите хранилище"), QStringLiteral("error"));
		return;
	}

	const auto directory = d->service->backupDirectory(alias);
	if (directory.isEmpty() || !QDir().mkpath(directory)) {
		const auto error = tr("Не удалось открыть каталог копий хранилища «%1»").arg(alias);
		d->setStatus(error, QStringLiteral("error"));
		emit errorOccurred(error);
		return;
	}

	QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
}
