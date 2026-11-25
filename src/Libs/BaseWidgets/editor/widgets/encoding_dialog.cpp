#include "Libs/BaseWidgets/editor/widgets/encoding_dialog.h"
#include "Libs/BaseWidgets/editor/widgets/code_editor_widget.h"

#include <QComboBox>
#include <QCompleter>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

EncodingDialog::EncodingDialog(const QString& currentEncoding, const QString& hint,
	bool allowResave, QWidget* parent)
	: QDialog(parent) {
	setObjectName(QStringLiteral("TextEditorEncodingDialog"));
	setWindowTitle(tr("Кодировка документа"));
	setModal(true);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Выберите кодировку:"), this));

	combo = new QComboBox(this);
	combo->setObjectName(QStringLiteral("TextEditorEncodingCombo"));
	for (const auto& info : CodeEditorWidget::availableEncodings()) {
		combo->addItem(info.title, info.name);
	}
	// Поиск по названию: поле редактируемое, фильтр — «содержит».
	combo->setEditable(true);
	combo->setInsertPolicy(QComboBox::NoInsert);
	if (auto* completer = combo->completer()) {
		completer->setCompletionMode(QCompleter::PopupCompletion);
		completer->setFilterMode(Qt::MatchContains);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
	}
	const int current = combo->findData(currentEncoding);
	combo->setCurrentIndex(current >= 0 ? current : 0);
	layout->addWidget(combo);

	auto* hintLabel = new QLabel(hint, this);
	hintLabel->setObjectName(QStringLiteral("TextEditorEncodingHint"));
	hintLabel->setWordWrap(true);
	layout->addWidget(hintLabel);

	auto* buttons = new QHBoxLayout();
	auto* reopenButton = new QPushButton(tr("Переоткрыть в выбранной кодировке"), this);
	reopenButton->setObjectName(QStringLiteral("TextEditorEncodingReopen"));
	buttons->addWidget(reopenButton);

	if (allowResave) {
		auto* resaveButton = new QPushButton(
			tr("Пересохранить в указанную кодировку"), this);
		resaveButton->setObjectName(QStringLiteral("TextEditorEncodingResave"));
		buttons->addWidget(resaveButton);
		connect(resaveButton, &QPushButton::clicked, this, [this] {
			selectedAction = Action::Resave;
			accept();
		});
	}

	auto* cancelButton = new QPushButton(tr("Отмена"), this);
	cancelButton->setObjectName(QStringLiteral("TextEditorEncodingCancel"));
	buttons->addStretch();
	buttons->addWidget(cancelButton);
	layout->addLayout(buttons);

	connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(reopenButton, &QPushButton::clicked, this, [this] {
		selectedAction = Action::Reopen;
		accept();
	});
}

QString EncodingDialog::selectedEncoding() const {
	const int index = combo ? combo->currentIndex() : -1;
	return index >= 0 ? combo->itemData(index).toString() : QString();
}

EncodingDialog::Action EncodingDialog::action() const {
	return selectedAction;
}
