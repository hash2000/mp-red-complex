#include "Content/SystemRouter/widgets/system_router_script_dialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class SystemRouterScriptDialog::Private {
public:
	explicit Private(SystemRouterScriptDialog* parent)
		: q(parent) {
	}

	SystemRouterScriptDialog* q;
	QList<SystemRouterScriptPreview> previews;
	QListWidget* list = nullptr;
	QLabel* title = nullptr;
	QLabel* version = nullptr;
	QLabel* description = nullptr;
	QPlainTextEdit* code = nullptr;
	QLabel* warning = nullptr;
	QPushButton* startButton = nullptr;
	QPushButton* viewButton = nullptr;
	QPushButton* cancelButton = nullptr;
	int currentIndex = 0;

	QString displayName(const SystemRouterScriptPreview& preview) const {
		if (!preview.propertyName.trimmed().isEmpty()) {
			return preview.propertyName;
		}
		if (!preview.command.description.trimmed().isEmpty()) {
			return preview.command.description;
		}
		return preview.commandId;
	}

	void select(int index) {
		if (previews.isEmpty()) {
			currentIndex = -1;
			return;
		}

		currentIndex = qBound(0, index, previews.size() - 1);
		const auto& preview = previews.at(currentIndex);

		title->setText(displayName(preview));
		version->setText(SystemRouterScriptDialog::tr("Версия обработчика: %1")
			.arg(preview.commandFound ? QString::number(preview.command.version)
				: QStringLiteral("—")));
		description->setText(preview.command.description.trimmed().isEmpty()
			? SystemRouterScriptDialog::tr("Описание не задано")
			: preview.command.description);
		code->setPlainText(preview.renderedScript);

		QStringList messages;
		if (!preview.error.isEmpty()) {
			messages.append(preview.error);
		}
		if (!preview.missingParameters.isEmpty()) {
			messages.append(SystemRouterScriptDialog::tr("Не найдены параметры: %1")
				.arg(preview.missingParameters.join(QStringLiteral(", "))));
		}
		warning->setText(messages.join(QStringLiteral("\n")));
		warning->setVisible(!messages.isEmpty());

		if (startButton) {
			startButton->setEnabled(preview.commandFound);
		}
		if (viewButton) {
			viewButton->setEnabled(true);
		}
	}
};

SystemRouterScriptDialog::SystemRouterScriptDialog(
	const QList<SystemRouterScriptPreview>& previews, QWidget* parent)
	: QDialog(parent)
	, d(std::make_unique<Private>(this)) {
	d->previews = previews;

	setWindowTitle(tr("Выбор скрипта"));
	setObjectName(QStringLiteral("SystemRouterScriptDialog"));
	resize(820, 520);

	auto* root = new QVBoxLayout(this);
	auto* content = new QHBoxLayout();
	root->addLayout(content, 1);

	d->list = new QListWidget(this);
	d->list->setObjectName(QStringLiteral("SystemRouterScriptList"));
	d->list->setMinimumWidth(220);
	content->addWidget(d->list, 1);

	auto* right = new QVBoxLayout();
	content->addLayout(right, 3);

	d->title = new QLabel(this);
	d->title->setObjectName(QStringLiteral("SystemRouterScriptTitle"));
	right->addWidget(d->title);

	d->version = new QLabel(this);
	d->version->setObjectName(QStringLiteral("SystemRouterScriptVersion"));
	right->addWidget(d->version);

	d->description = new QLabel(this);
	d->description->setObjectName(QStringLiteral("SystemRouterScriptDescription"));
	d->description->setWordWrap(true);
	right->addWidget(d->description);

	d->code = new QPlainTextEdit(this);
	d->code->setObjectName(QStringLiteral("SystemRouterScriptCode"));
	d->code->setReadOnly(true);
	right->addWidget(d->code, 1);

	d->warning = new QLabel(this);
	d->warning->setObjectName(QStringLiteral("SystemRouterScriptWarning"));
	d->warning->setWordWrap(true);
	right->addWidget(d->warning);

	// Нижняя строка диалога: «Запуск» и «Просмотр» слева, «Отмена» справа.
	// Цвета кнопок задаёт тема (system-router.qss).
	auto* buttonsRow = new QHBoxLayout();
	buttonsRow->setContentsMargins(0, 0, 0, 0);
	buttonsRow->setSpacing(6);

	d->startButton = new QPushButton(tr("Запуск"), this);
	d->startButton->setObjectName(QStringLiteral("SystemRouterScriptStartButton"));
	d->startButton->setDefault(true);

	d->viewButton = new QPushButton(tr("Просмотр"), this);
	d->viewButton->setObjectName(QStringLiteral("SystemRouterScriptViewButton"));
	d->viewButton->setToolTip(tr("Открыть параметры узла без запуска скрипта"));
	d->viewButton->setAutoDefault(false);

	d->cancelButton = new QPushButton(tr("Отмена"), this);
	d->cancelButton->setObjectName(QStringLiteral("SystemRouterScriptCancelButton"));
	d->cancelButton->setAutoDefault(false);

	buttonsRow->addWidget(d->startButton);
	buttonsRow->addWidget(d->viewButton);
	buttonsRow->addStretch(1);
	buttonsRow->addWidget(d->cancelButton);
	root->addLayout(buttonsRow);

	connect(d->startButton, &QPushButton::clicked, this, &QDialog::accept);
	connect(d->viewButton, &QPushButton::clicked, this, [this]() {
		done(SystemRouterScriptDialog::ViewResult);
	});
	connect(d->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(d->list, &QListWidget::currentRowChanged, this, [this](int row) {
		d->select(row);
	});

	for (int i = 0; i < d->previews.size(); i++) {
		const auto& preview = d->previews.at(i);
		auto* item = new QListWidgetItem(d->displayName(preview), d->list);
		item->setToolTip(preview.command.description);
	}

	if (d->previews.isEmpty()) {
		d->list->addItem(tr("Скрипты не найдены"));
		d->title->setText(tr("Скрипты не найдены"));
		d->version->setText(tr("Версия обработчика: —"));
		d->description->setText(tr("У узла нет привязанных команд"));
		d->warning->setVisible(false);
		d->startButton->setEnabled(false);
		d->viewButton->setEnabled(false);
	}
	else {
		d->list->setCurrentRow(0);
		d->select(0);
	}
}

SystemRouterScriptDialog::~SystemRouterScriptDialog() = default;

const SystemRouterScriptPreview& SystemRouterScriptDialog::selectedPreview() const {
	static const SystemRouterScriptPreview empty;
	if (d->currentIndex < 0 || d->currentIndex >= d->previews.size()) {
		return empty;
	}

	return d->previews.at(d->currentIndex);
}
