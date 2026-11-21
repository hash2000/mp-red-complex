#include "Libs/BaseWidgets/properties/tab_color_editor.h"

#include <QColorDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QToolButton>

TabColorEditor::TabColorEditor(QWidget* parent)
	: QWidget(parent)
	, _dialogTitle(tr("Цвет вкладки")) {
	setObjectName(QStringLiteral("TabColorEditor"));

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	_button = new QToolButton(this);
	_button->setObjectName(QStringLiteral("TabColorButton"));
	_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	_button->setToolTip(tr("Выбрать цвет вкладки"));

	_reset = new QToolButton(this);
	_reset->setObjectName(QStringLiteral("TabColorReset"));
	_reset->setText(QStringLiteral("✕"));
	_reset->setToolTip(tr("Сбросить цвет вкладки"));

	connect(_button, &QToolButton::clicked, this, &TabColorEditor::chooseColor);
	connect(_reset, &QToolButton::clicked, this, [this] {
		if (!_color.isValid()) {
			return;
		}

		_color = QColor();
		updateView();
		emit colorChanged(_color);
	});

	layout->addWidget(_button, 1);
	layout->addWidget(_reset);
	updateView();
}

TabColorEditor::~TabColorEditor() = default;

QColor TabColorEditor::color() const {
	return _color;
}

void TabColorEditor::setColor(const QColor& color) {
	if (_color == color) {
		return;
	}

	_color = color;
	updateView();
}

QString TabColorEditor::dialogTitle() const {
	return _dialogTitle;
}

void TabColorEditor::setDialogTitle(const QString& title) {
	_dialogTitle = title;
}

void TabColorEditor::chooseColor() {
	const auto initial = _color.isValid() ? _color : palette().color(QPalette::Highlight);
	const auto picked = QColorDialog::getColor(initial, this, _dialogTitle);
	if (!picked.isValid()) {
		return;
	}

	_color = picked;
	updateView();
	emit colorChanged(_color);
}

void TabColorEditor::updateView() {
	if (!_button || !_reset) {
		return;
	}

	_button->setText(_color.isValid() ? _color.name() : tr("не задан"));

	QPixmap swatch(14, 14);
	swatch.fill(_color.isValid() ? _color : Qt::transparent);
	_button->setIcon(QIcon(swatch));
	_reset->setEnabled(_color.isValid());
}
