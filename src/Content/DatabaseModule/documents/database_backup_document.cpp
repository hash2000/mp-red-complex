#include "Content/DatabaseModule/documents/database_backup_document.h"
#include "Content/DatabaseModule/services/database_backup_service.h"
#include "Content/DatabaseModule/widgets/database_backup_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <optional>

namespace {
const QString kStoreVariable = QStringLiteral("store");
}

struct DatabaseBackupDocument::Private {
	DatabaseBackupWidget* widget = nullptr;
	/// true — документ сам пишет переменную контекста или выбирает хранилище:
	/// обратная реакция на изменение в это время не нужна.
	bool applying = false;
};

DatabaseBackupDocument::DatabaseBackupDocument(DatabaseBackupService* service, QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	setTitle(tr("Резервное копирование баз данных"));
	setGlyph(QStringLiteral("💾"));

	d->widget = new DatabaseBackupWidget(service, this);
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(d->widget);

	connect(d->widget, &DatabaseBackupWidget::storeSelected, this,
		[this](const QString& alias) {
			if (d->applying) {
				return;
			}

			d->applying = true;
			setVariable(kStoreVariable, alias);
			d->applying = false;
			emit propertiesChanged();
		});
	connect(d->widget, &DatabaseBackupWidget::statusChanged, this,
		[this](const QString&, bool) {
			emit propertiesChanged();
		});
	connect(d->widget, &DatabaseBackupWidget::errorOccurred, this,
		[this](const QString& message) {
			printMessage(message, true);
		});
	connect(d->widget, &DatabaseBackupWidget::backupCreated, this,
		[this](const QString& alias, const QString& path) {
			printMessage(tr("Создана резервная копия базы «%1»: %2")
				.arg(alias, path), false);
		});
	connect(d->widget, &DatabaseBackupWidget::databaseRestored, this,
		[this](const QString& alias, const QString& path) {
			printMessage(tr("База «%1» восстановлена из копии: %2").arg(alias, path), false);
		});
}

DatabaseBackupDocument::~DatabaseBackupDocument() = default;

DatabaseBackupWidget* DatabaseBackupDocument::widget() const {
	return d->widget;
}

void DatabaseBackupDocument::printMessage(const QString& message, bool error) {
	auto context = this->context();
	auto* controller = context ? context->commandController() : nullptr;
	if (!controller) {
		return;
	}

	const auto text = tr("database-backup: %1").arg(message);
	if (error) {
		controller->printError(text);
	}
	else {
		controller->printSuccess(text);
	}
}

void DatabaseBackupDocument::onContextAttached() {
	const auto alias = hasVariable(kStoreVariable)
		? variable(kStoreVariable).toString()
		: QString();
	if (alias.isEmpty()) {
		emit propertiesChanged();
		return;
	}

	d->applying = true;
	if (!d->widget->selectStore(alias)) {
		// Хранилище могло появиться после создания документа: перечитываем
		// список и пробуем выбрать его ещё раз.
		d->widget->refresh();
		d->widget->selectStore(alias);
	}
	d->applying = false;
	emit propertiesChanged();
}

void DatabaseBackupDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
	if (name != kStoreVariable || d->applying) {
		return;
	}

	d->applying = true;
	d->widget->selectStore(value.toString());
	d->applying = false;
	emit propertiesChanged();
}

QWidget* DatabaseBackupDocument::createPropertiesEditor(QWidget* parent) {
	auto* editor = DocumentWidget::createPropertiesEditor(parent);
	auto* form = qobject_cast<QFormLayout*>(editor->layout());
	if (!form || !d->widget) {
		return editor;
	}

	auto* service = d->widget->service();
	const auto alias = d->widget->selectedStore();
	const auto info = (service && !alias.isEmpty()) ? service->store(alias) : std::nullopt;

	auto* storeLabel = new QLabel(alias.isEmpty() ? tr("не выбрано") : alias, editor);
	storeLabel->setObjectName(QStringLiteral("DatabaseBackupStoreValue"));
	storeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto* pathLabel = new QLabel(info ? info->path : QStringLiteral("—"), editor);
	pathLabel->setObjectName(QStringLiteral("DatabaseBackupPathValue"));
	pathLabel->setWordWrap(true);
	pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	auto* encryptedLabel = new QLabel(info && info->encrypted ? tr("да") : tr("нет"), editor);
	auto* backupsLabel = new QLabel(info ? QString::number(info->backupCount) : QStringLiteral("—"),
		editor);

	const auto directory = service && !alias.isEmpty()
		? service->backupDirectory(alias)
		: QString();
	auto* directoryLabel = new QLabel(directory.isEmpty() ? QStringLiteral("—") : directory, editor);
	directoryLabel->setObjectName(QStringLiteral("DatabaseBackupDirectoryValue"));
	directoryLabel->setWordWrap(true);
	directoryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	form->addRow(tr("Хранилище:"), storeLabel);
	form->addRow(tr("Файл базы:"), pathLabel);
	form->addRow(tr("Шифрование:"), encryptedLabel);
	form->addRow(tr("Копий:"), backupsLabel);
	form->addRow(tr("Каталог копий:"), directoryLabel);
	return editor;
}

void DatabaseBackupDocument::refresh() {
	if (d->widget) {
		d->widget->refresh();
	}
	emit propertiesChanged();
}

bool DatabaseBackupDocument::save() {
	// Операции с копиями выполняются сразу и в контексте не откладываются.
	return true;
}
