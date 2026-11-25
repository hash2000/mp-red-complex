#include "Libs/BaseWidgets/editor/widgets/code_editor_widget.h"
#include "Libs/BaseWidgets/editor/language_info.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin.h"
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
constexpr int kLineLengthCheckDelayMs = 150;
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
	QTimer* lineLengthTimer = nullptr;

	/// Кодировка чтения/записи файла (имя кодека QTextCodec).
	QString encoding = QStringLiteral("UTF-8");

	/// Режим переноса, выбранный пользователем, и фактическое состояние.
	LineWrapPolicy wrapPolicy = LineWrapPolicy::Auto;
	bool wrapped = false;
	/// Режим подсветки (ручной выбор имеет приоритет над авто-запретом).
	HighlightingPolicy highlightPolicy = HighlightingPolicy::Auto;
	/// Есть ли строки длиннее предела автопереноса.
	bool hasLongLines = false;
	/// Длина самой длинной строки, если уже известна (файл/пакетный текст).
	/// -1 — нужно пересчитать обходом блоков.
	int cachedMaxLineLength = -1;

	void setupUI();
	void setupStyling();

	/// Настроить режим подсветки под текст, который сейчас будет вставлен.
	void prepareBulkText(const QString& text);
	/// Привести режим подсветки в соответствие с текущим документом.
	void syncHighlightingMode();
	void scheduleVisibleHighlight(int delayMs = kVisibleHighlightDelayMs);
	void updateVisibleHighlight();
	/// Применить перенос строк и запрет подсветки по длине строк документа.
	void applyLineLengthPolicies();
	/// Длина самой длинной строки (с ранним выходом за пределы подсветки).
	int documentMaxLineLength() const;
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

	lineLengthTimer = new QTimer(q);
	lineLengthTimer->setSingleShot(true);
	lineLengthTimer->setInterval(kLineLengthCheckDelayMs);
	QObject::connect(lineLengthTimer, &QTimer::timeout, q, [this] {
		applyLineLengthPolicies();
	});
}

int CodeEditorWidget::Private::documentMaxLineLength() const {
	if (cachedMaxLineLength >= 0) {
		return cachedMaxLineLength;
	}

	QTextDocument* doc = q->document();
	if (!doc) {
		return 0;
	}

	int maxLength = 0;
	const int limit = Highlighter::MaxHighlightedBlockLength;
	for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
		maxLength = qMax(maxLength, block.length() - 1);
		if (maxLength > limit) {
			return maxLength;
		}
	}

	return maxLength;
}

void CodeEditorWidget::Private::applyLineLengthPolicies() {
	const int maxLength = documentMaxLineLength();

	bool wrap = false;
	switch (wrapPolicy) {
	case LineWrapPolicy::On:
		wrap = true;
		break;
	case LineWrapPolicy::Auto:
		wrap = maxLength > kLongLineWrapLimit;
		break;
	case LineWrapPolicy::Off:
		// Приоритет пользователя: переноса нет совсем. Предупреждение
		// о возможных тормозах выводит документ.
		wrap = false;
		break;
	}

	if (wrap != wrapped) {
		wrapped = wrap;
		q->setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth
			: QPlainTextEdit::NoWrap);
		emit q->lineWrappingChanged(wrap);
	}

	const bool longLines = maxLength > kLongLineWrapLimit;
	if (longLines != hasLongLines) {
		hasLongLines = longLines;
		emit q->longLinesChanged(longLines);
	}

	// Слишком длинная строка отключает подсветку в автоматическом режиме:
	// иначе она пересчитывается при каждой правке и подвешивает ввод.
	// Ручной выбор пользователя имеет приоритет над этим запретом.
	bool enabled = true;
	switch (highlightPolicy) {
	case HighlightingPolicy::Auto:
		enabled = maxLength <= Highlighter::MaxHighlightedBlockLength;
		break;
	case HighlightingPolicy::On:
		enabled = true;
		break;
	case HighlightingPolicy::Off:
		enabled = false;
		break;
	}

	if (highlighter->isEnabled() != enabled) {
		highlighter->setEnabled(enabled);
		emit q->highlightingEnabledChanged(enabled);
	}
}

void CodeEditorWidget::Private::setupStyling() {
	// Оформление задаётся темой (stylesheets/code-editor.qss),
	// здесь остаётся только моноширинный шрифт редактора.
	q->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void CodeEditorWidget::Private::prepareBulkText(const QString& text) {
	// Метрики пакетного текста считаем за один проход, пока строка уже в памяти:
	// иначе на 140 МБ обход блоков в GUI-потоке заметен.
	int maxLineLength = 0;
	int lineStart = 0;
	for (int i = 0; i < text.size(); i++) {
		if (text.at(i) != QLatin1Char('\n')) {
			continue;
		}

		maxLineLength = qMax(maxLineLength, i - lineStart);
		lineStart = i + 1;
	}
	maxLineLength = qMax(maxLineLength, text.size() - lineStart);
	cachedMaxLineLength = maxLineLength;

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
	if (d->lineLengthTimer) {
		d->lineLengthTimer->stop();
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

	// Открытие файла — новый документ: ручной выбор подсветки сбрасывается,
	// дальше работает авто-режим (как при обычном открытии).
	d->highlightPolicy = HighlightingPolicy::Auto;

	// Предыдущее чтение больше не должно попадать в редактор.
	if (d->reader) {
		d->reader->disconnect(this);
		d->reader->deleteLater();
		d->reader = nullptr;
	}

	auto* reader = new FileReader();
	d->reader = reader;
	d->readerFailed = false;
	reader->setEncoding(d->encoding);
	// Крупные файлы вставляем реже и большими пачками: меньше edit-блоков
	// и пересчётов раскладки при загрузке.
	reader->setChunkSize(info.size() > kPreloadLazyByteLimit ? 5000 : 500);

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
		if (success) {
			// Метрики уже посчитаны в фоновом потоке — сохраняем их до
			// applyLineLengthPolicies(), чтобы не обходить блоки на GUI.
			d->cachedMaxLineLength = reader->maxLineLength();
		}
		d->reader = nullptr;
		d->readerFailed = false;
		d->readerThread = nullptr;
		setUndoRedoEnabled(true);
		d->syncHighlightingMode();
		d->applyLineLengthPolicies();
		d->scheduleVisibleHighlight(0);
		emit fileOpened(path, success);
	});
	connect(thread, &QThread::finished, reader, &QObject::deleteLater);
	connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	connect(reader, &FileReader::blockRead, this, &CodeEditorWidget::onBlockRead);
	connect(reader, &FileReader::metricsComputed, this,
		[this](int, qint64, int maxLineLength) {
			d->cachedMaxLineLength = maxLineLength;
		});

	// Загрузку файла не нужно записывать в undo: это экономит память
	// на больших текстах (история всё равно очищается при открытии).
	d->cachedMaxLineLength = -1;
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

CodeEditorWidget::LineWrapPolicy CodeEditorWidget::lineWrapPolicy() const {
	return d->wrapPolicy;
}

void CodeEditorWidget::setLineWrapPolicy(LineWrapPolicy policy) {
	if (d->wrapPolicy == policy) {
		return;
	}

	d->wrapPolicy = policy;
	d->applyLineLengthPolicies();
}

bool CodeEditorWidget::hasLongLines() const {
	return d->documentMaxLineLength() > kLongLineWrapLimit;
}

CodeEditorWidget::DocumentMetrics CodeEditorWidget::documentMetrics() const {
	DocumentMetrics metrics;
	if (!document()) {
		return metrics;
	}

	metrics.lines = document()->blockCount();
	metrics.characters = qMax<qint64>(0, document()->characterCount() - 1);
	metrics.maxLineLength = d->documentMaxLineLength();
	return metrics;
}

CodeEditorWidget::HighlightingPolicy CodeEditorWidget::highlightingPolicy() const {
	return d->highlightPolicy;
}

void CodeEditorWidget::setHighlightingPolicy(HighlightingPolicy policy) {
	if (d->highlightPolicy == policy) {
		return;
	}

	d->highlightPolicy = policy;
	d->applyLineLengthPolicies();
}

bool CodeEditorWidget::isHighlightingEnabled() const {
	return d->highlighter && d->highlighter->isEnabled();
}

int CodeEditorWidget::autoWrapLineLimit() {
	return kLongLineWrapLimit;
}

int CodeEditorWidget::maxHighlightedLineLength() {
	return Highlighter::MaxHighlightedBlockLength;
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

QList<CodeEditorWidget::EncodingInfo> CodeEditorWidget::availableEncodings() {
	// Порядок: сначала Unicode, затем кириллица и остальные популярные.
	return {
		{ QStringLiteral("UTF-8"), tr("UTF-8") },
		{ QStringLiteral("UTF-16"), tr("UTF-16 (BOM)") },
		{ QStringLiteral("UTF-16LE"), tr("UTF-16 LE") },
		{ QStringLiteral("UTF-16BE"), tr("UTF-16 BE") },
		{ QStringLiteral("UTF-32"), tr("UTF-32 (BOM)") },
		{ QStringLiteral("UTF-32LE"), tr("UTF-32 LE") },
		{ QStringLiteral("UTF-32BE"), tr("UTF-32 BE") },
		{ QStringLiteral("ISO-8859-1"), tr("Latin1 (ISO-8859-1)") },
		{ QStringLiteral("Windows-1251"), tr("Windows-1251 (кириллица)") },
		{ QStringLiteral("Windows-1252"), tr("Windows-1252 (Западная Европа)") },
		{ QStringLiteral("Windows-1250"), tr("Windows-1250 (Центральная Европа)") },
		{ QStringLiteral("KOI8-R"), tr("KOI8-R") },
		{ QStringLiteral("KOI8-U"), tr("KOI8-U") },
		{ QStringLiteral("IBM866"), tr("IBM866 (DOS, кириллица)") },
		{ QStringLiteral("ISO-8859-5"), tr("ISO-8859-5 (кириллица)") },
		{ QStringLiteral("ISO-8859-15"), tr("ISO-8859-15") },
		{ QStringLiteral("Shift-JIS"), tr("Shift-JIS") },
		{ QStringLiteral("EUC-JP"), tr("EUC-JP") },
		{ QStringLiteral("GBK"), tr("GBK") },
		{ QStringLiteral("GB18030"), tr("GB18030") },
		{ QStringLiteral("Big5"), tr("Big5") },
		{ QStringLiteral("EUC-KR"), tr("EUC-KR") },
		{ QStringLiteral("System"), tr("Системная (ANSI)") },
	};
}

QString CodeEditorWidget::encoding() const {
	return d->encoding;
}

void CodeEditorWidget::setEncoding(const QString& encoding) {
	if (d->encoding == encoding) {
		return;
	}

	d->encoding = encoding;
	emit encodingChanged(encoding);
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

	// На больших документах полный обход блоков после каждой правки дорог:
	// оставляем ранее посчитанную длину самой длинной строки.
	if (d->cachedMaxLineLength >= 0
		&& document()->blockCount() <= kFullHighlightBlockLimit) {
		d->cachedMaxLineLength = -1;
	}

	// Длину строк проверяем с задержкой: обход всех блоков не должен
	// выполняться на каждое нажатие клавиши.
	if (d->lineLengthTimer) {
		d->lineLengthTimer->start();
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
	// Новый текст — новый документ: ручной выбор подсветки сбрасывается,
	// дальше снова работает авто-режим.
	d->highlightPolicy = HighlightingPolicy::Auto;

	if (text.isEmpty()) {
		d->cachedMaxLineLength = 0;
		clear();
		d->applyLineLengthPolicies();
		return;
	}

	// Форматирование часто возвращает тот же текст: не пересоздаём документ.
	if (text == toPlainText()) {
		return;
	}

	d->prepareBulkText(text);
	QPlainTextEdit::setPlainText(text);
	d->syncHighlightingMode();
	d->applyLineLengthPolicies();

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
