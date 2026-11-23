#include "Content/CodeEditorWidget/widgets/code_editor_widget.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/highlighter.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include "Libs/DataStream/file_reader.h"
#include "Libs/Base/extensions/text_edit_extensions.h"

#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QPointer>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QThread>
#include <QTimer>

namespace {
/// Полная подсветка допустима только для небольших документов: она идёт
/// по всем строкам и для сотен килобайт занимает секунды. Верхняя граница
/// выбрана так, чтобы один полный проход укладывался в ~150-200 мс.
constexpr int kFullHighlightBlockLimit = 2000;
constexpr qsizetype kFullHighlightCharLimit = 1024 * 1024; // 1 МиБ в UTF-16

/// Файл больше этого размера читается сразу в ленивом режиме: иначе
/// QSyntaxHighlighter подсветит все вставленные строки до того, как станет
/// известен реальный размер документа.
constexpr qint64 kPreloadLazyByteLimit = 128 * 1024;

/// Сколько строк подсвечивать вокруг видимой области.
constexpr int kVisibleBlockMargin = 25;
/// Верхняя оценка видимых строк до появления раскладки.
constexpr int kDefaultVisibleBlockSpan = 250;
/// Задержка перед подсветкой видимой области: прокрутка не должна
/// выполнять регулярные выражения на каждом шаге.
constexpr int kVisibleHighlightDelayMs = 30;
/// Форматирование внешней утилитой не должно занимать интерфейс бесконечно.
constexpr int kFormatTimeoutMs = 120000;

/// Строка длиннее этого предела включает автоперенос.
///
/// Без переноса QPlainTextEdit пересчитывает и перерисовывает строку целиком,
/// поэтому стоимость кадра растёт линейно с длиной строки: на строках в сотни
/// символов изменение размера и прокрутка уже заметны, на минифицированной
/// разметке (десятки килобайт в строке) — это секунды на кадр. С переносом
/// стоимость не зависит от длины строки.
constexpr int kLongLineWrapLimit = 256;

/// Задержка перед повторной проверкой длины строк после правки.
constexpr int kWrapCheckDelayMs = 150;
}

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;
	HighlightingPluginManager* pluginManager = nullptr;
	FormatterPluginManager* formatterManager = nullptr;
	FormatterPlugin* currentFormatter = nullptr;
	FileReader* reader = nullptr;
	QPointer<QThread> readerThread;
	bool readerFailed = false;

	QTimer* visibleTimer = nullptr;
	QTimer* formatTimeout = nullptr;
	QTimer* wrapTimer = nullptr;

	/// Автоперенос включён пользователем (по умолчанию) и активен сейчас.
	bool autoWrapEnabled = true;
	bool autoWrapped = false;

	void setupUI();
	void setupStyling();

	/// Настроить режим подсветки под текст, который сейчас будет вставлен.
	void prepareBulkText(const QString& text);
	/// Привести режим подсветки в соответствие с текущим документом.
	void syncHighlightingMode();
	void scheduleVisibleHighlight(int delayMs = kVisibleHighlightDelayMs);
	void updateVisibleHighlight();
	void updateLineWrapping();
	void finishFormatting();
};

void CodeEditorWidget::Private::setupUI() {
	using namespace Extensions;

	highlighter = new Highlighter(pluginManager, q->document());
	highlighter->setLanguage(QStringLiteral("txt"));
	TextEdit::setTabDistance(q, 2);

	// Компактные отступы документа: для текстового редактора они не нужны.
	q->document()->setDocumentMargin(4);

	visibleTimer = new QTimer(q);
	visibleTimer->setSingleShot(true);
	QObject::connect(visibleTimer, &QTimer::timeout, q, [this] {
		updateVisibleHighlight();
	});

	formatTimeout = new QTimer(q);
	formatTimeout->setSingleShot(true);
	formatTimeout->setInterval(kFormatTimeoutMs);
	QObject::connect(formatTimeout, &QTimer::timeout, q, [this] {
		q->onFormattingFailed(QObject::tr("Превышено время форматирования"));
	});

	QObject::connect(q->verticalScrollBar(), &QScrollBar::valueChanged, q, [this] {
		scheduleVisibleHighlight();
	});
	// Пока пользователь тащит ползунок, подсветку не считаем: она
	// выполняется один раз после отпускания.
	QObject::connect(q->verticalScrollBar(), &QScrollBar::sliderReleased, q, [this] {
		scheduleVisibleHighlight(0);
	});

	wrapTimer = new QTimer(q);
	wrapTimer->setSingleShot(true);
	wrapTimer->setInterval(kWrapCheckDelayMs);
	QObject::connect(wrapTimer, &QTimer::timeout, q, [this] {
		updateLineWrapping();
	});
}

void CodeEditorWidget::Private::updateLineWrapping() {
	QTextDocument* doc = q->document();
	if (!doc) {
		return;
	}

	if (!autoWrapEnabled) {
		if (!autoWrapped) {
			return;
		}

		autoWrapped = false;
		q->setLineWrapMode(QPlainTextEdit::NoWrap);
		emit q->lineWrappingChanged(false);
		return;
	}

	// Ищем хотя бы одну строку длиннее предела: дальше документ не важен.
	bool hasLongLine = false;
	for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
		if (block.length() > kLongLineWrapLimit) {
			hasLongLine = true;
			break;
		}
	}

	if (hasLongLine == autoWrapped) {
		return;
	}

	autoWrapped = hasLongLine;
	q->setLineWrapMode(hasLongLine ? QPlainTextEdit::WidgetWidth
		: QPlainTextEdit::NoWrap);
	emit q->lineWrappingChanged(hasLongLine);
}

void CodeEditorWidget::Private::setupStyling() {
	// Оформление задаётся темой (stylesheets/code-editor.qss),
	// здесь остаётся только моноширинный шрифт редактора.
	q->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void CodeEditorWidget::Private::prepareBulkText(const QString& text) {
	if (!highlighter) {
		return;
	}

	const bool lazy = text.size() > kFullHighlightCharLimit ||
		text.count(QLatin1Char('\n')) >= kFullHighlightBlockLimit;
	highlighter->setLazyMode(lazy);
	if (lazy) {
		// Пока раскладки нет, считаем видимыми первые строки.
		highlighter->setVisibleBlockRange(0, kDefaultVisibleBlockSpan);
	}
}

void CodeEditorWidget::Private::syncHighlightingMode() {
	if (!highlighter || !q->document()) {
		return;
	}

	// Пока файл читается, режим уже выбран заранее по размеру файла:
	// понижать его нельзя, иначе первые пачки строк подсветятся целиком,
	// а вместе с ними — полный rehighlight на каждом куске.
	if (reader) {
		if (!highlighter->isLazyMode()) {
			highlighter->setLazyMode(true);
			highlighter->setVisibleBlockRange(0, kDefaultVisibleBlockSpan);
		}
		return;
	}

	const bool lazy = q->document()->characterCount() > kFullHighlightCharLimit ||
		q->document()->blockCount() > kFullHighlightBlockLimit;
	if (lazy == highlighter->isLazyMode()) {
		return;
	}

	highlighter->setLazyMode(lazy);
	if (lazy) {
		highlighter->setVisibleBlockRange(0, kDefaultVisibleBlockSpan);
		updateVisibleHighlight();
		return;
	}

	// Документ стал небольшим: возвращаем полную подсветку (один проход).
	highlighter->rehighlight();
}

void CodeEditorWidget::Private::scheduleVisibleHighlight(int delayMs) {
	if (!highlighter || !highlighter->isLazyMode() || !visibleTimer) {
		return;
	}

	// Во время перетаскивания ползунка прокрутка идёт непрерывно:
	// подсветим после отпускания (sliderReleased).
	if (q->verticalScrollBar()->isSliderDown() && delayMs > 0) {
		return;
	}

	visibleTimer->start(delayMs);
}

void CodeEditorWidget::Private::updateVisibleHighlight() {
	if (!highlighter || !highlighter->isLazyMode()) {
		return;
	}

	QTextDocument* doc = q->document();
	if (!doc) {
		return;
	}

	int firstNumber = 0;
	int lastNumber = 0;
	const QTextBlock first = q->firstVisibleBlock();
	if (first.isValid()) {
		firstNumber = first.blockNumber();
		lastNumber = firstNumber;

		const QRect viewportRect = q->viewport()->rect();
		for (QTextBlock block = first; block.isValid(); block = block.next()) {
			const QRectF geometry = q->blockBoundingGeometry(block).translated(q->contentOffset());
			if (geometry.top() > viewportRect.bottom()) {
				break;
			}

			lastNumber = block.blockNumber();
			if (lastNumber - firstNumber > kDefaultVisibleBlockSpan) {
				break;
			}
		}
	}

	firstNumber = qMax(0, firstNumber - kVisibleBlockMargin);
	lastNumber = qMin(doc->blockCount() - 1, lastNumber + kVisibleBlockMargin);

	highlighter->setVisibleBlockRange(firstNumber, lastNumber);
	highlighter->refreshVisibleBlocks();
}

void CodeEditorWidget::Private::finishFormatting() {
	if (formatTimeout) {
		formatTimeout->stop();
	}

	if (currentFormatter) {
		FormatterPlugin* formatter = currentFormatter;
		currentFormatter = nullptr;
		formatter->stop();
	}
}

CodeEditorWidget::CodeEditorWidget(
	HighlightingPluginManager* pluginManager,
	FormatterPluginManager* formatterManager,
	QWidget* parent)
	: QPlainTextEdit(parent)
	, d(std::make_unique<Private>(this)) {
	d->pluginManager = pluginManager;
	d->formatterManager = formatterManager;
	d->setupUI();
	d->setupStyling();

	connect(this, &QPlainTextEdit::textChanged, this, &CodeEditorWidget::onTextChanged);
}

CodeEditorWidget::~CodeEditorWidget() {
	if (d->visibleTimer) {
		d->visibleTimer->stop();
	}
	if (d->formatTimeout) {
		d->formatTimeout->stop();
	}
	if (d->wrapTimer) {
		d->wrapTimer->stop();
	}

	if (d->currentFormatter) {
		d->currentFormatter->stop();
		d->currentFormatter->disconnect(this);
		d->currentFormatter = nullptr;
	}

	// Чтение идёт в отдельном потоке: не даём ему пережить редактор.
	if (d->reader) {
		d->reader->disconnect(this);
		d->reader->deleteLater();
	}
	if (d->readerThread) {
		d->readerThread->disconnect(this);
		d->readerThread->quit();
		d->readerThread->wait(2000);
	}
}

void CodeEditorWidget::setHoghlighterByPath(const QString& path) {
	QFileInfo info(path);
	d->highlighter->setLanguage(info.suffix().toLower());
}

void CodeEditorWidget::openFile(const QString& path) {
	QFileInfo info(path);
	if (path.isEmpty() || !info.exists() || !info.isFile() || !info.isReadable()) {
		emit fileOpened(path, false);
		return;
	}

	setHoghlighterByPath(path);

	// Предыдущее чтение больше не должно попадать в редактор.
	if (d->reader) {
		d->reader->disconnect(this);
		d->reader->deleteLater();
		d->reader = nullptr;
	}

	auto* reader = new FileReader();
	d->reader = reader;
	d->readerFailed = false;

	QThread* thread = new QThread(this);
	d->readerThread = thread;
	reader->moveToThread(thread);

	connect(thread, &QThread::started, reader, [reader, path]() {
		reader->readFileAsync(path);
	});
	connect(reader, &FileReader::finished, thread, &QThread::quit);
	connect(reader, &FileReader::failed, this, [this, reader](const QString& errorMessage) {
		if (d->reader == reader) {
			d->readerFailed = true;
		}
		qWarning() << "CodeEditorWidget:" << errorMessage;
	});
	connect(reader, &FileReader::finished, this, [this, reader, path]() {
		// Файл уже переоткрыт другим вызовом — это устаревшее чтение.
		if (d->reader != reader) {
			return;
		}

		const bool success = !d->readerFailed;
		d->reader = nullptr;
		d->readerFailed = false;
		d->readerThread = nullptr;
		setUndoRedoEnabled(true);
		d->syncHighlightingMode();
		d->updateLineWrapping();
		d->scheduleVisibleHighlight(0);
		emit fileOpened(path, success);
	});
	connect(thread, &QThread::finished, reader, &QObject::deleteLater);
	connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	connect(reader, &FileReader::blockRead, this, &CodeEditorWidget::onBlockRead);

	// Загрузку файла не нужно записывать в undo: это экономит память
	// на больших текстах (история всё равно очищается при открытии).
	setUndoRedoEnabled(false);
	clear();

	// Большой файл читается сразу в ленивом режиме: иначе подсветка
	// успеет обработать все вставленные строки.
	if (info.size() > kPreloadLazyByteLimit && d->highlighter) {
		d->highlighter->setLazyMode(true);
		d->highlighter->setVisibleBlockRange(0, kDefaultVisibleBlockSpan);
	}

	thread->start();
}

bool CodeEditorWidget::setLanguage(const QString& lang) {
	if (lang.isEmpty()) {
		return false;
	}

	return d->highlighter->setLanguage(lang.toLower());
}

bool CodeEditorWidget::setContentType(const QString& type) {
	if (type.isEmpty()) {
		return false;
	}

	const auto contents = type.split(";", Qt::SkipEmptyParts);
	for (const auto tp : contents) {
		if (d->highlighter->setContentType(tp.toLower().trimmed())) {
			return true;
		}
	}

	return false;
}

bool CodeEditorWidget::formatDocument() {
	// Повторное нажатие во время форматирования не запускает второй процесс.
	if (d->currentFormatter) {
		return true;
	}

	const auto plugin = d->highlighter->plugin();
	if (!plugin) {
		return false;
	}

	const auto formatter = d->formatterManager
		? d->formatterManager->formatter(plugin->languageInfo()) : nullptr;
	if (!formatter) {
		return false;
	}

	d->currentFormatter = formatter;
	connect(formatter, &FormatterPlugin::formattingFailed,
		this, &CodeEditorWidget::onFormattingFailed, Qt::UniqueConnection);
	connect(formatter, &FormatterPlugin::formattingFinished,
		this, &CodeEditorWidget::onFormattingFinished, Qt::UniqueConnection);

	d->formatTimeout->start();
	emit formattingStarted();

	d->currentFormatter->formatAsync(toPlainText());
	return true;
}

bool CodeEditorWidget::isFormatting() const {
	return d->currentFormatter != nullptr;
}

bool CodeEditorWidget::isHighlightingLazy() const {
	return d->highlighter && d->highlighter->isLazyMode();
}

bool CodeEditorWidget::isAutoWrapEnabled() const {
	return d->autoWrapEnabled;
}

int CodeEditorWidget::autoWrapLineLimit() {
	return kLongLineWrapLimit;
}

void CodeEditorWidget::setAutoWrapEnabled(bool enabled) {
	if (d->autoWrapEnabled == enabled) {
		return;
	}

	d->autoWrapEnabled = enabled;
	d->updateLineWrapping();
}

int CodeEditorWidget::fontSize() const {
	const int pointSize = font().pointSize();
	return pointSize > 0 ? pointSize : 10;
}

void CodeEditorWidget::setFontSize(int size) {
	if (size <= 0 || size == font().pointSize()) {
		return;
	}

	QFont current = font();
	current.setPointSize(size);
	setFont(current);
	emit fontSizeChanged(size);
}

const HighlightingPlugin* CodeEditorWidget::highlightingPlugin() const {
	return d->highlighter->plugin();
}

void CodeEditorWidget::onTextChanged() {
	if (d->currentFormatter) {
		FormatterPlugin* formatter = d->currentFormatter;
		d->finishFormatting();
		formatter->disconnect(this);
		emit formattingCompleted();
	}

	d->syncHighlightingMode();
	d->scheduleVisibleHighlight();

	// Длину строк проверяем с задержкой: обход всех блоков не должен
	// выполняться на каждое нажатие клавиши.
	if (d->wrapTimer) {
		d->wrapTimer->start();
	}
}

void CodeEditorWidget::onFormattingFailed(const QString& errorMessage) {
	if (!d->currentFormatter) {
		return;
	}

	FormatterPlugin* formatter = d->currentFormatter;
	d->finishFormatting();
	formatter->disconnect(this);

	emit formattedFailed(errorMessage);
	emit formattingCompleted();
}

void CodeEditorWidget::onFormattingFinished(const QString& formattedText) {
	if (!d->currentFormatter) {
		return;
	}

	FormatterPlugin* formatter = d->currentFormatter;
	d->finishFormatting();
	formatter->disconnect(this);

	const QTextCursor oldCursor = textCursor();
	const int position = oldCursor.position();
	const int anchor = oldCursor.anchor();
	const int scrollBarValue = verticalScrollBar()->value();
	const bool changed = formattedText != toPlainText();

	setText(formattedText);

	// Позиция курсора и прокрутка сохраняются: форматирование не должно
	// перебрасывать пользователя в начало документа.
	const int length = formattedText.size();
	QTextCursor cursor = textCursor();
	const int anchorPosition = qBound(0, anchor, length);
	const int cursorPosition = qBound(0, position, length);
	if (anchorPosition != cursorPosition) {
		cursor.setPosition(anchorPosition);
		cursor.setPosition(cursorPosition, QTextCursor::KeepAnchor);
	}
	else {
		cursor.setPosition(cursorPosition);
	}
	setTextCursor(cursor);
	verticalScrollBar()->setValue(scrollBarValue);

	// setPlainText() сбрасывает флаг изменённости, но форматирование — правка.
	if (changed) {
		document()->setModified(true);
	}
	d->scheduleVisibleHighlight(0);

	emit formattingCompleted();
}

void CodeEditorWidget::onBlockRead(const QStringList& lines) {
	if (!d->reader || sender() != d->reader || lines.isEmpty()) {
		return;
	}

	// Одна вставка на пачку строк вместо вставки построчно: меньше
	// уведомлений документу и пересчётов подсветки.
	QString chunk = lines.join(QLatin1Char('\n'));
	chunk += QLatin1Char('\n');

	QTextCursor cursor(document());
	cursor.movePosition(QTextCursor::End);
	cursor.beginEditBlock();
	cursor.insertText(chunk);
	cursor.endEditBlock();

	d->syncHighlightingMode();
}

void CodeEditorWidget::setText(const QString& text) {
	if (text.isEmpty()) {
		clear();
		d->updateLineWrapping();
		return;
	}

	// Форматирование часто возвращает тот же текст: не пересоздаём документ.
	if (text == toPlainText()) {
		return;
	}

	d->prepareBulkText(text);
	QPlainTextEdit::setPlainText(text);
	d->syncHighlightingMode();
	d->updateLineWrapping();

	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::Start);
	setTextCursor(cursor);

	d->scheduleVisibleHighlight(0);
}

void CodeEditorWidget::resizeEvent(QResizeEvent* event) {
	QPlainTextEdit::resizeEvent(event);
	d->scheduleVisibleHighlight();
}

void CodeEditorWidget::wheelEvent(QWheelEvent* event) {
	const bool ctrlPressed = (event->modifiers() & Qt::ControlModifier);
	if (!ctrlPressed) {
		QPlainTextEdit::wheelEvent(event);
		return;
	}

	const int delta = event->angleDelta().y();
	if (delta != 0) {
		setFontSize(qMax(1, fontSize() + (delta > 0 ? 1 : -1)));
	}

	event->accept();
}
