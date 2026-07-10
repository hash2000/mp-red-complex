#include "Content/CodeEditorWidget/code_editor_widget.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/highlighter.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include "Libs/DataStream/file_reader.h"
#include "Libs/Base/extensions/text_edit_extensions.h"

#include <QThread>
#include <QFileInfo>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QScrollBar>

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;
	HighlightingPluginManager* pluginManager;
	FormatterPluginManager* formatterManager;
	FormatterPlugin* currentFormatter = nullptr;
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

CodeEditorWidget::~CodeEditorWidget() = default;

void CodeEditorWidget::Private::setupUI() {
	using namespace Extensions;

	highlighter = new Highlighter(pluginManager, q->document());
	highlighter->setLanguage("txt");
	TextEdit::setTabDistance(q, 2);
}

void CodeEditorWidget::Private::setupStyling() {
	q->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	QString styleSheet = R"(
        QTextEdit {
            background-color: #1e1e1e;
            color: #d4d4d4;
            border: 1px solid #3e3e42;
            border-radius: 3px;
        }
    )";
	q->setStyleSheet(styleSheet);
}

void CodeEditorWidget::setHoghlighterByPath(const QString& path) {
	QFileInfo info(path);
	d->highlighter->setLanguage(info.suffix().toLower());
}

void CodeEditorWidget::openFile(const QString& path) {
	setHoghlighterByPath(path);
	QThread* thread = new QThread(this);
	FileReader* reader = new FileReader();
	reader->moveToThread(thread);

	connect(thread, &QThread::started, reader, [reader, path]() {
		reader->readFileAsync(path);
	});
	connect(reader, &FileReader::finished, thread, &QThread::quit);
	connect(reader, &FileReader::finished, thread, [this]() { d->highlighter->rehighlight(); });
	connect(thread, &QThread::finished, reader, &QObject::deleteLater);
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

void CodeEditorWidget::formatDocument() {
	const auto plugin = d->highlighter->plugin();
	if (!plugin) {
		return;
	}

	const auto formatter = d->formatterManager->formatter(plugin->languageInfo());
	if (!formatter) {
		return;
	}

	d->currentFormatter = formatter;

	connect(formatter, &FormatterPlugin::formattingFailed, this, &CodeEditorWidget::onFormattingFailed);
	connect(formatter, &FormatterPlugin::formattingFinished, this, &CodeEditorWidget::onFormattingFinished);

	d->currentFormatter->formatAsync(toPlainText());
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
	if (lines.isEmpty()) {
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

	QFont currentFont = font();
	int currentSize = currentFont.pointSize();
	const int ZOOM_STEP = 1;
	if (event->angleDelta().y() > 0) {
		currentFont.setPointSize(currentSize + ZOOM_STEP);
	}
	else if (event->angleDelta().y() < 0) {
		int newSize = currentSize - ZOOM_STEP;
		if (newSize < 1) {
			newSize = 1;
		}

		currentFont.setPointSize(newSize);
	}

	setFont(currentFont);
	event->accept();
}
