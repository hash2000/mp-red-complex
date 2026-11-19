#include "Content/CodeEditorWidget/widgets/code_editor_widget.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/highlighter.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include "Libs/DataStream/file_reader.h"
#include "Libs/Base/extensions/text_edit_extensions.h"

#include <QFont>
#include <QFontDatabase>
#include <QFileInfo>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QPointer>
#include <QScrollBar>
#include <QThread>

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
	QString previusSuffix;

	void setupUI();
	void setupStyling();
};

CodeEditorWidget::CodeEditorWidget(
	HighlightingPluginManager* pluginManager,
	FormatterPluginManager* formatterManager,
	QWidget* parent)
	: QTextEdit(parent)
	, d(std::make_unique<Private>(this)) {
	d->pluginManager = pluginManager;
	d->formatterManager = formatterManager;
	d->setupUI();
	d->setupStyling();

	connect(this, &QTextEdit::textChanged, this, &CodeEditorWidget::onTextChanged);
}

CodeEditorWidget::~CodeEditorWidget() {
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

void CodeEditorWidget::Private::setupUI() {
	using namespace Extensions;

	highlighter = new Highlighter(pluginManager, q->document());
	highlighter->setLanguage("txt");
	TextEdit::setTabDistance(q, 2);
}

void CodeEditorWidget::Private::setupStyling() {
	// Оформление задаётся темой (stylesheets/code-editor.qss),
	// здесь остаётся только моноширинный шрифт редактора.
	q->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
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
		d->highlighter->rehighlight();
		emit fileOpened(path, success);
	});
	connect(thread, &QThread::finished, reader, &QObject::deleteLater);
	connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	connect(reader, &FileReader::blockRead, this, &CodeEditorWidget::onBlockRead);

	clear();
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
	const auto plugin = d->highlighter->plugin();
	if (!plugin) {
		return false;
	}

	const auto formatter = d->formatterManager->formatter(plugin->languageInfo());
	if (!formatter) {
		return false;
	}

	d->currentFormatter = formatter;

	connect(formatter, &FormatterPlugin::formattingFailed, this, &CodeEditorWidget::onFormattingFailed);
	connect(formatter, &FormatterPlugin::formattingFinished, this, &CodeEditorWidget::onFormattingFinished);

	d->currentFormatter->formatAsync(toPlainText());
	return true;
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
	if (!d->currentFormatter) {
		return;
	}

	disconnect(d->currentFormatter);
	d->currentFormatter->stop();
	d->currentFormatter = nullptr;
}

void CodeEditorWidget::onFormattingFailed(const QString& errorMessage) {
	if (!d->currentFormatter) {
		return;
	}

	disconnect(d->currentFormatter);
	d->currentFormatter = nullptr;
	emit formattedFailed(errorMessage);
}

void CodeEditorWidget::onFormattingFinished(const QString& formattedText) {
	if (!d->currentFormatter) {
		return;
	}

	disconnect(d->currentFormatter);
	d->currentFormatter = nullptr;
	QTextCursor cursor = textCursor();
	int cursorPos = cursor.position();
	int scrollBarValue = verticalScrollBar()->value();

	setUpdatesEnabled(false);
	setText(formattedText);
	setUpdatesEnabled(true);

	cursor.setPosition(qMin(cursorPos, formattedText.length()));
	setTextCursor(cursor);
	verticalScrollBar()->setValue(scrollBarValue);
	d->highlighter->rehighlight();
}

void CodeEditorWidget::onBlockRead(const QStringList& lines) {
	if (!d->reader || sender() != d->reader || lines.isEmpty()) {
		return;
	}

	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.beginEditBlock();

	for (const QString& line : lines) {
		cursor.insertText(line + "\n");
	}

	cursor.endEditBlock();
	setTextCursor(cursor);
}

void CodeEditorWidget::setText(const QString& text) {
	if (text.isEmpty()) {
		clear();
		return;
	}

	QTextCursor cursor = textCursor();
	cursor.beginEditBlock();
	cursor.select(QTextCursor::Document);
	cursor.insertText(text);
	cursor.endEditBlock();
	cursor.movePosition(QTextCursor::Start);
	setTextCursor(cursor);
}

void CodeEditorWidget::wheelEvent(QWheelEvent* event) {
	const bool ctrlPressed = (event->modifiers() & Qt::ControlModifier);
	if (!ctrlPressed) {
		QTextEdit::wheelEvent(event);
		return;
	}

	const int delta = event->angleDelta().y();
	if (delta != 0) {
		setFontSize(qMax(1, fontSize() + (delta > 0 ? 1 : -1)));
	}

	event->accept();
}
