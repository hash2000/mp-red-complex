#include "Content/CodeEditorWidget/code_editor_widget.h"
#include "Content/CodeEditorWidget/highlights/highlighter.h"
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "DataStream/file_reader.h"

#include <QThread>
#include <QFileInfo>

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;
	HighlightingPlugin* lastPlugin = nullptr;
	HighlightingPluginManager* pluginManager;
	QString path;
	QString previusSuffix;

	void setupUI();
	void setupStyling();
};

CodeEditorWidget::CodeEditorWidget(HighlightingPluginManager* pluginManager, QWidget* parent)
	: QTextEdit(parent)
	, d(std::make_unique<Private>(this)) {
	d->pluginManager = pluginManager;
	d->setupUI();
	d->setupStyling();
}

CodeEditorWidget::~CodeEditorWidget() = default;

void CodeEditorWidget::Private::setupUI() {
	highlighter = new Highlighter(pluginManager, q->document());
	highlighter->setLanguage("txt");
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

void CodeEditorWidget::setPath(const QString& path) {
	d->path = path;
	QFileInfo info(d->path);
	d->highlighter->setLanguage(info.suffix().toLower());
	reloadFile();	
}

void CodeEditorWidget::reloadFile() {
	QThread* thread = new QThread(this);
	FileReader* reader = new FileReader();
	reader->moveToThread(thread);

	connect(thread, &QThread::started, reader, [reader, path = d->path]() {
		reader->readFileAsync(path);
	});
	connect(reader, &FileReader::finished, thread, &QThread::quit);
	connect(thread, &QThread::finished, reader, &QObject::deleteLater);
	connect(reader, &FileReader::blockRead, this, &CodeEditorWidget::onBlockRead);

	clear();
	thread->start();
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
}

void CodeEditorWidget::setText(const QString& text) {
	clear();
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.beginEditBlock();
	cursor.insertText(text);
	cursor.endEditBlock();
}

