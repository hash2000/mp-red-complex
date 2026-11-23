#include "Content/CodeEditorWidget/widgets/code_editor_search_panel.h"

#include <QPlainTextEdit>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>

namespace {
/// Задержка перед пересчётом совпадений при вводе и правке текста.
constexpr int kRefreshDelayMs = 120;

/// Больше этого числа вхождений подсвечивать в редакторе не нужно:
/// каждая ExtraSelection — отдельный курсор, их отрисовка на большом
/// документе заметно тормозит прокрутку. Счётчик совпадений при этом
/// остаётся полным.
constexpr int kMaxHighlights = 1000;
}

class CodeEditorSearchPanel::Private {
public:
	Private(CodeEditorSearchPanel* parent) : q(parent) {}
	CodeEditorSearchPanel* q;

	QPlainTextEdit* editor = nullptr;
	QLineEdit* input = nullptr;
	QLabel* counter = nullptr;
	QToolButton* previousButton = nullptr;
	QToolButton* nextButton = nullptr;
	QToolButton* closeButton = nullptr;
	QTimer refreshTimer;

	QList<Match> matches;
	int current = -1;

	bool regularExpression = false;
	bool simplifiedExpression = false;
	bool caseSensitive = false;
	bool wholeWord = false;

	void resetMatches();
	void refreshDebounced();
};

void CodeEditorSearchPanel::Private::resetMatches() {
	matches.clear();
	current = -1;
}

void CodeEditorSearchPanel::Private::refreshDebounced() {
	refreshTimer.start();
}

void CodeEditorSearchPanel::setupUi() {
	setObjectName(QStringLiteral("CodeEditorSearchPanel"));
	setAttribute(Qt::WA_StyledBackground);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(6, 4, 6, 4);
	layout->setSpacing(4);

	d->input = new QLineEdit(this);
	d->input->setObjectName(QStringLiteral("CodeEditorSearchInput"));
	d->input->setPlaceholderText(tr("Найти"));
	d->input->setClearButtonEnabled(true);
	layout->addWidget(d->input, 1);

	d->counter = new QLabel(this);
	d->counter->setObjectName(QStringLiteral("CodeEditorSearchCount"));
	d->counter->setMinimumWidth(64);
	d->counter->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(d->counter);

	auto makeButton = [this, layout](const QString& glyph, const QString& tooltip) {
		auto* button = new QToolButton(this);
		button->setObjectName(QStringLiteral("CodeEditorSearchButton"));
		button->setText(glyph);
		button->setToolTip(tooltip);
		button->setToolButtonStyle(Qt::ToolButtonTextOnly);
		button->setCursor(Qt::PointingHandCursor);
		button->setFocusPolicy(Qt::NoFocus);
		layout->addWidget(button);
		return button;
	};

	d->previousButton = makeButton(QStringLiteral("↑"), tr("Предыдущее совпадение"));
	d->nextButton = makeButton(QStringLiteral("↓"), tr("Следующее совпадение"));
	d->closeButton = makeButton(QStringLiteral("✕"), tr("Закрыть поиск"));

	connect(d->previousButton, &QToolButton::clicked, this, &CodeEditorSearchPanel::activatePrevious);
	connect(d->nextButton, &QToolButton::clicked, this, &CodeEditorSearchPanel::activateNext);
	connect(d->closeButton, &QToolButton::clicked, this, [this] {
		emit closed();
	});

	connect(d->input, &QLineEdit::textChanged, this, [this] {
		d->refreshDebounced();
	});

	// Enter — следующее совпадение, Shift+Enter — предыдущее.
	connect(d->input, &QLineEdit::returnPressed, this, [this] {
		if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
			activatePrevious();
		}
		else {
			activateNext();
		}
	});

	connect(&d->refreshTimer, &QTimer::timeout, this, [this] {
		refresh();
	});
	d->refreshTimer.setSingleShot(true);
	d->refreshTimer.setInterval(kRefreshDelayMs);

	updateCounter();
}

CodeEditorSearchPanel::CodeEditorSearchPanel(QPlainTextEdit* editor, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->editor = editor;
	setupUi();

	if (d->editor) {
		// Правка текста (в том числе форматирование) пересчитывает совпадения.
		connect(d->editor->document(), &QTextDocument::contentsChanged, this, [this] {
			if (isVisible()) {
				d->refreshDebounced();
			}
		});
	}
}

CodeEditorSearchPanel::~CodeEditorSearchPanel() = default;

void CodeEditorSearchPanel::setOption(Option option, bool enabled) {
	switch (option) {
		case Option::RegularExpression:
			d->regularExpression = enabled;
			break;
		case Option::SimplifiedExpression:
			d->simplifiedExpression = enabled;
			break;
		case Option::CaseSensitive:
			d->caseSensitive = enabled;
			break;
		case Option::WholeWord:
			d->wholeWord = enabled;
			break;
	}

	if (isVisible()) {
		refresh();
	}
}

bool CodeEditorSearchPanel::option(Option option) const {
	switch (option) {
		case Option::RegularExpression:
			return d->regularExpression;
		case Option::SimplifiedExpression:
			return d->simplifiedExpression;
		case Option::CaseSensitive:
			return d->caseSensitive;
		case Option::WholeWord:
			return d->wholeWord;
	}

	return false;
}

void CodeEditorSearchPanel::focusSearch() {
	show();
	d->input->setFocus();
	d->input->selectAll();
	refresh();
}

void CodeEditorSearchPanel::clearSearch() {
	d->refreshTimer.stop();
	d->resetMatches();
	d->counter->clear();

	if (d->editor) {
		d->editor->setExtraSelections({});
	}

	updateCounter();
}

int CodeEditorSearchPanel::matchCount() const {
	return static_cast<int>(d->matches.size());
}

void CodeEditorSearchPanel::setQuery(const QString& text) {
	d->input->setText(text);
	refresh();
}

QString CodeEditorSearchPanel::query() const {
	return d->input->text();
}

int CodeEditorSearchPanel::highlightedCount() const {
	return d->editor ? static_cast<int>(d->editor->extraSelections().size()) : 0;
}

QString CodeEditorSearchPanel::buildPattern(bool& valid) const {
	valid = true;

	const auto query = d->input->text();
	if (query.isEmpty()) {
		return { };
	}

	// Регулярное выражение берётся как есть.
	if (d->regularExpression) {
		return query;
	}

	QString pattern;
	if (d->simplifiedExpression) {
		// Простой текст, но последовательности \n, \t, \r означают переводы.
		for (int i = 0; i < query.size(); i++) {
			const QChar current = query.at(i);
			if (current == QLatin1Char('\\') && i + 1 < query.size()) {
				const QChar next = query.at(i + 1);
				if (next == QLatin1Char('n')) {
					pattern += QStringLiteral("\\n");
					i++;
					continue;
				}
				if (next == QLatin1Char('t')) {
					pattern += QStringLiteral("\\t");
					i++;
					continue;
				}
				if (next == QLatin1Char('r')) {
					pattern += QStringLiteral("\\r");
					i++;
					continue;
				}
			}

			pattern += QRegularExpression::escape(QString(current));
		}
	}
	else {
		pattern = QRegularExpression::escape(query);
	}

	if (d->wholeWord) {
		pattern = QStringLiteral("(?<!\\w)(?:%1)(?!\\w)").arg(pattern);
	}

	return pattern;
}

void CodeEditorSearchPanel::refresh() {
	if (!d->editor) {
		return;
	}

	d->resetMatches();

	bool valid = true;
	const auto pattern = buildPattern(valid);
	if (pattern.isEmpty()) {
		d->editor->setExtraSelections({});
		updateCounter();
		return;
	}

	QRegularExpression::PatternOptions options;
	if (!d->caseSensitive) {
		options |= QRegularExpression::CaseInsensitiveOption;
	}

	const QRegularExpression expression(pattern, options);
	if (!expression.isValid()) {
		d->editor->setExtraSelections({});
		d->counter->setText(tr("ошибка"));
		return;
	}

	const auto text = d->editor->toPlainText();
	auto it = expression.globalMatch(text);
	while (it.hasNext()) {
		const auto match = it.next();
		if (match.capturedLength() <= 0) {
			continue;
		}

		d->matches.append({ static_cast<int>(match.capturedStart()),
			static_cast<int>(match.capturedLength()) });
	}

	if (!d->matches.isEmpty()) {
		// Начинаем с первого совпадения после текущей позиции курсора.
		const int cursorPosition = d->editor->textCursor().position();
		d->current = 0;
		for (int i = 0; i < d->matches.size(); i++) {
			if (d->matches.at(i).start >= cursorPosition) {
				d->current = i;
				break;
			}
		}
	}

	applyHighlights();
	updateCounter();
}

void CodeEditorSearchPanel::updateCounter() {
	if (!d->counter) {
		return;
	}

	const int total = static_cast<int>(d->matches.size());
	if (total == 0) {
		d->counter->setText(QStringLiteral("0/0"));
		return;
	}

	d->counter->setText(QStringLiteral("%1/%2").arg(d->current + 1).arg(total));
}

void CodeEditorSearchPanel::applyHighlights() {
	if (!d->editor) {
		return;
	}

	// Цвета берутся из палитры темы, чтобы подсветка не выбивалась из оформления.
	const auto highlight = d->editor->palette().color(QPalette::Highlight);
	QColor matchesColor = highlight;
	matchesColor.setAlpha(70);
	QColor currentColor = highlight;
	currentColor.setAlpha(160);

	QList<QTextEdit::ExtraSelection> selections;
	const int first = d->matches.size() > kMaxHighlights
		? qMax(0, d->current - kMaxHighlights / 2) : 0;
	const int last = qMin(d->matches.size(), first + kMaxHighlights);
	selections.reserve(last - first);

	for (int i = first; i < last; i++) {
		const auto& match = d->matches.at(i);
		QTextCursor cursor(d->editor->document());
		cursor.setPosition(match.start);
		cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);

		QTextEdit::ExtraSelection selection;
		selection.cursor = cursor;
		selection.format.setBackground(i == d->current ? currentColor : matchesColor);
		selections.append(selection);
	}

	d->editor->setExtraSelections(selections);
}

void CodeEditorSearchPanel::moveToCurrent() {
	if (!d->editor || d->current < 0 || d->current >= d->matches.size()) {
		return;
	}

	const auto& match = d->matches.at(d->current);
	QTextCursor cursor(d->editor->document());
	cursor.setPosition(match.start);
	cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);
	d->editor->setTextCursor(cursor);
	d->editor->ensureCursorVisible();

	// ensureCursorVisible ориентируется на позицию курсора (конец выделения),
	// поэтому при длинном совпадении слева видна только его хвост.
	// Дополнительно выравниваем видимую область по началу совпадения.
	QTextCursor startCursor(d->editor->document());
	startCursor.setPosition(match.start);
	const auto rect = d->editor->cursorRect(startCursor);
	auto* horizontal = d->editor->horizontalScrollBar();
	if (!horizontal || rect.isNull()) {
		return;
	}

	const int margin = 8;
	const int viewportWidth = d->editor->viewport()->width();
	int offset = 0;
	if (rect.left() < margin) {
		offset = rect.left() - margin;
	}
	else if (rect.right() > viewportWidth - margin) {
		offset = rect.right() - viewportWidth + margin;
	}

	if (offset != 0) {
		horizontal->setValue(horizontal->value() + offset);
	}
}

void CodeEditorSearchPanel::activateNext() {
	if (d->matches.isEmpty()) {
		return;
	}

	d->current = (d->current + 1) % d->matches.size();
	applyHighlights();
	updateCounter();
	moveToCurrent();
}

void CodeEditorSearchPanel::activatePrevious() {
	if (d->matches.isEmpty()) {
		return;
	}

	d->current = (d->current - 1 + d->matches.size()) % d->matches.size();
	applyHighlights();
	updateCounter();
	moveToCurrent();
}

void CodeEditorSearchPanel::keyPressEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Escape) {
		emit closed();
		event->accept();
		return;
	}

	QWidget::keyPressEvent(event);
}
