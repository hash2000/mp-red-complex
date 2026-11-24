#include "Content/TextEditorModule/widgets/search_bar.h"

#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

SearchBar::SearchBar(QWidget* parent)
	: QWidget(parent) {
	setObjectName(QStringLiteral("TextEditorSearchBar"));
	setAttribute(Qt::WA_StyledBackground);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(6, 4, 6, 4);
	layout->setSpacing(4);

	_input = new QLineEdit(this);
	_input->setObjectName(QStringLiteral("TextEditorSearchInput"));
	_input->setPlaceholderText(tr("Найти"));
	_input->setClearButtonEnabled(true);
	_input->installEventFilter(this);
	layout->addWidget(_input, 1);

	_counter = new QLabel(this);
	_counter->setObjectName(QStringLiteral("TextEditorSearchCount"));
	_counter->setMinimumWidth(64);
	_counter->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(_counter);

	auto makeButton = [this, layout](const QString& glyph, const QString& tooltip) {
		auto* button = new QToolButton(this);
		button->setObjectName(QStringLiteral("TextEditorSearchButton"));
		button->setText(glyph);
		button->setToolTip(tooltip);
		button->setToolButtonStyle(Qt::ToolButtonTextOnly);
		button->setCursor(Qt::PointingHandCursor);
		button->setFocusPolicy(Qt::NoFocus);
		layout->addWidget(button);
		return button;
	};

	auto* previousButton = makeButton(QStringLiteral("↑"), tr("Предыдущее совпадение"));
	auto* nextButton = makeButton(QStringLiteral("↓"), tr("Следующее совпадение"));
	auto* closeButton = makeButton(QStringLiteral("✕"), tr("Закрыть поиск"));

	connect(previousButton, &QToolButton::clicked, this, &SearchBar::previousRequested);
	connect(nextButton, &QToolButton::clicked, this, &SearchBar::nextRequested);
	connect(closeButton, &QToolButton::clicked, this, &SearchBar::closeRequested);
	connect(_input, &QLineEdit::textChanged, this, &SearchBar::queryChanged);
	connect(_input, &QLineEdit::returnPressed, this, [this] {
		if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
			emit previousRequested();
		}
		else {
			emit nextRequested();
		}
	});

	setCounter(-1, 0);
}

QLineEdit* SearchBar::input() const {
	return _input;
}

QLabel* SearchBar::counter() const {
	return _counter;
}

void SearchBar::focusSearch() {
	_input->setFocus();
	_input->selectAll();
}

void SearchBar::setCounter(int currentIndex, int total) {
	if (!_counter) {
		return;
	}

	if (total <= 0) {
		_counter->setText(QStringLiteral("0/0"));
		return;
	}

	const int position = currentIndex >= 0 ? currentIndex + 1 : 0;
	_counter->setText(QStringLiteral("%1/%2").arg(position).arg(total));
}

void SearchBar::setBusy() {
	if (_counter) {
		_counter->setText(QStringLiteral("…"));
	}
}

void SearchBar::setMessage(const QString& text) {
	if (_counter) {
		_counter->setText(text);
	}
}

bool SearchBar::eventFilter(QObject* watched, QEvent* event) {
	if (watched == _input && event->type() == QEvent::KeyPress) {
		auto* keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Escape) {
			emit closeRequested();
			return true;
		}
	}

	return QWidget::eventFilter(watched, event);
}
