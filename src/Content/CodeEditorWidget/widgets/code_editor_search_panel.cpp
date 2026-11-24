#include "Content/CodeEditorWidget/widgets/code_editor_search_panel.h"

#include <QFutureWatcher>
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
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <atomic>
#include <memory>

namespace {
/// Задержка перед пересчётом совпадений при вводе и правке текста.
constexpr int kRefreshDelayMs = 120;

/// Больше этого числа вхождений подсвечивать в редакторе не нужно:
/// каждая ExtraSelection — отдельный курсор, их отрисовка на большом
/// документе заметно тормозит прокрутку. Счётчик совпадений при этом
/// остаётся полным.
constexpr int kMaxHighlights = 1000;

/// Документы больше этого размера ищутся в фоне (regex по 140 МБ
/// на GUI-потоке вешает окно на минуты).
constexpr qint64 kAsyncSearchCharLimit = 2 * 1024 * 1024;

/// Сколько совпадений храним для переходов. Полный счётчик считается
/// всегда, но миллионы смещений в памяти не нужны.
constexpr int kMaxStoredMatches = 200000;

/// Сколько вхождений подсвечивать в видимой области при фоновом поиске.
constexpr int kMaxVisibleHighlights = 2000;

/// Совпадение, найденное в фоновом потоке.
struct WorkerMatch {
	int start = 0;
	int length = 0;
};

/// Результат фонового поиска.
struct SearchOutcome {
	QVector<WorkerMatch> matches;
	int total = 0;
	bool truncated = false;
	bool cancelled = false;
};

/// Поиск по снимку текста. Отмена проверяется между совпадениями, чтобы
/// при смене запроса старый проход быстро завершился.
SearchOutcome runSearch(QString text, QRegularExpression expression,
	const std::shared_ptr<std::atomic_bool>& cancel) {
	SearchOutcome outcome;
	auto it = expression.globalMatch(text);
	while (it.hasNext()) {
		if (cancel && (outcome.total & 0x3FFF) == 0 && cancel->load()) {
			outcome.cancelled = true;
			return outcome;
		}

		const auto match = it.next();
		if (match.capturedLength() <= 0) {
			continue;
		}

		outcome.total++;
		if (outcome.matches.size() < kMaxStoredMatches) {
			outcome.matches.append({ static_cast<int>(match.capturedStart()),
				static_cast<int>(match.capturedLength()) });
		}
		else {
			outcome.truncated = true;
		}
	}

	return outcome;
}
} // namespace

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
	int totalMatches = 0;

	// Фоновый поиск по большим документам.
	QFutureWatcher<SearchOutcome>* watcher = nullptr;
	std::shared_ptr<std::atomic_bool> cancelFlag;
	QString snapshot;
	bool snapshotValid = false;
	bool largeSearch = false;
	bool busy = false;
	quint64 generation = 0;
	quint64 activeGeneration = 0;
	bool hasPending = false;
	QRegularExpression pendingExpression;

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
	totalMatches = 0;
}

void CodeEditorSearchPanel::Private::refreshDebounced() {
	refreshTimer.start();
}

void CodeEditorSearchPanel::startLargeSearch(const QRegularExpression& expression) {
	if (!d->editor) {
		return;
	}

	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}

	const quint64 generation = ++d->generation;
	if (d->busy) {
		d->pendingExpression = expression;
		d->hasPending = true;
		return;
	}

	// Снимок текста делается один раз на правку: toPlainText() для 140 МБ
	// стоит сотни миллисекунд, а regex по нему на GUI-потоке — минуты.
	if (!d->snapshotValid) {
		d->snapshot = d->editor->toPlainText();
		d->snapshotValid = true;
	}

	auto cancel = std::make_shared<std::atomic_bool>(false);
	d->cancelFlag = cancel;
	d->busy = true;
	d->activeGeneration = generation;
	d->largeSearch = true;
	d->resetMatches();
	d->counter->setText(tr("…"));

	d->watcher->setFuture(QtConcurrent::run(
		[text = d->snapshot, expression, cancel] {
			return runSearch(text, expression, cancel);
		}));
}

void CodeEditorSearchPanel::finishLargeSearch() {
	d->busy = false;

	const bool isCurrent = d->activeGeneration == d->generation;
	const SearchOutcome outcome = d->watcher->result();
	if (isCurrent && !outcome.cancelled) {
		d->matches.clear();
		d->matches.reserve(outcome.matches.size());
		for (const auto& match : outcome.matches) {
			d->matches.append({ match.start, match.length });
		}

		d->totalMatches = outcome.total;
		d->current = -1;
		if (!d->matches.isEmpty()) {
			// Начинаем с первого совпадения после текущей позиции курсора.
			const int position = d->editor ? d->editor->textCursor().position() : 0;
			d->current = 0;
			for (int i = 0; i < d->matches.size(); i++) {
				if (d->matches.at(i).start >= position) {
					d->current = i;
					break;
				}
			}
		}

		applyHighlights();
		updateCounter();
	}

	if (d->hasPending) {
		const auto expression = d->pendingExpression;
		d->hasPending = false;
		startLargeSearch(expression);
	}
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

	d->watcher = new QFutureWatcher<SearchOutcome>(this);
	connect(d->watcher, &QFutureWatcher<SearchOutcome>::finished, this, [this] {
		finishLargeSearch();
	});

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
			// Снимок для фонового поиска устарел: следующий поиск снимет новый.
			d->snapshotValid = false;
			if (isVisible()) {
				d->refreshDebounced();
			}
		});

		// При фоновом поиске подсвечиваем только видимые совпадения:
		// при прокрутке набор ExtraSelections обновляется.
		if (auto* scroll = d->editor->verticalScrollBar()) {
			connect(scroll, &QScrollBar::valueChanged, this, [this] {
				if (d->largeSearch) {
					applyHighlights();
				}
			});
		}
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
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->hasPending = false;
	d->resetMatches();
	d->counter->clear();

	if (d->editor) {
		d->editor->setExtraSelections({});
	}

	updateCounter();
}

int CodeEditorSearchPanel::matchCount() const {
	return d->totalMatches;
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

	const bool largeDocument =
		d->editor->document()->characterCount() > kAsyncSearchCharLimit;
	if (largeDocument) {
		startLargeSearch(expression);
		return;
	}

	// Небольшой документ ищем сразу; фоновый результат больше не нужен.
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->hasPending = false;
	d->largeSearch = false;

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
	d->totalMatches = static_cast<int>(d->matches.size());

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

	const int total = d->totalMatches;
	if (total == 0) {
		d->counter->setText(QStringLiteral("0/0"));
		return;
	}

	const int position = d->current >= 0 ? d->current + 1 : 0;
	d->counter->setText(QStringLiteral("%1/%2").arg(position).arg(total));
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
	if (d->largeSearch) {
		applyVisibleHighlights(selections, matchesColor, currentColor);
		d->editor->setExtraSelections(selections);
		return;
	}

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

void CodeEditorSearchPanel::applyVisibleHighlights(
	QList<QTextEdit::ExtraSelection>& selections,
	const QColor& matchesColor, const QColor& currentColor) const {
	if (!d->editor || d->matches.isEmpty()) {
		return;
	}

	// Диапазон видимых символов: подсвечиваем только загруженный/видимый блок.
	int firstPosition = 0;
	int lastPosition = d->editor->document()->characterCount();
	if (auto* viewport = d->editor->viewport()) {
		const auto top = d->editor->cursorForPosition(QPoint(0, 0));
		const auto bottom = d->editor->cursorForPosition(
			QPoint(viewport->width(), qMax(0, viewport->height() - 1)));
		firstPosition = qMin(top.position(), bottom.position());
		lastPosition = qMax(top.position(), bottom.position());
	}

	const auto addSelection = [this, &selections, &matchesColor, &currentColor](
		const Match& match, bool current) {
		QTextCursor cursor(d->editor->document());
		cursor.setPosition(match.start);
		cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);

		QTextEdit::ExtraSelection selection;
		selection.cursor = cursor;
		selection.format.setBackground(current ? currentColor : matchesColor);
		selections.append(selection);
	};

	const auto begin = std::lower_bound(d->matches.cbegin(), d->matches.cend(),
		firstPosition, [](const Match& match, int value) { return match.start < value; });
	int added = 0;
	for (auto it = begin; it != d->matches.cend() && it->start <= lastPosition
		&& added < kMaxVisibleHighlights; ++it, ++added) {
		addSelection(*it, false);
	}

	// Текущее совпадение показываем всегда, даже если оно за пределами экрана.
	if (d->current >= 0 && d->current < d->matches.size()) {
		const auto& match = d->matches.at(d->current);
		if (match.start < firstPosition || match.start > lastPosition) {
			addSelection(match, true);
		}
		else {
			// Перекрашиваем текущее внутри видимого диапазона.
			for (auto& selection : selections) {
				if (selection.cursor.selectionStart() == match.start) {
					selection.format.setBackground(currentColor);
				}
			}
		}
	}
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
