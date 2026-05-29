#include "CodeEditorWidget/code_editor_widget.h"
#include "CodeEditorWidget/highlights/highlighter.h"
#include "CodeEditorWidget/highlights/plugins/i_highlighter_plugin.h"
#include "CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "DataStream/file_reader.h"

#include <QThread>
#include <QFileInfo>

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;
	IHighlightingPlugin* lastPlugin = nullptr;
	HighlightingPluginManager* pluginManager;
	QString path;
	QString previusSuffix;

	void setupUI();
	void setupHighlighterRules();
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
	highlighter = new Highlighter(q->document());
	setupHighlighterRules();
}

void CodeEditorWidget::Private::setupHighlighterRules() {
	if (!pluginManager) {
		return;
	}

	if (lastPlugin) {
		lastPlugin->uninstall(*highlighter);
	}

	const auto pluginPath = path.isEmpty() ? "plugin.txt" : path;
	const auto plugin = pluginManager->pluginForFile(pluginPath);
	if (!plugin) {
		return;
	}

	plugin->install(*highlighter);
	lastPlugin = plugin;
}

void CodeEditorWidget::Private::setupStyling() {
	//q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
	d->setupHighlighterRules();
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
	connect(reader, &FileReader::lineRead, this, &CodeEditorWidget::onAappendPlainText);

	clear();
	thread->start();
}

void CodeEditorWidget::onAappendPlainText(const QString& line) {
	QTextCursor cursor = textCursor();
	cursor.movePosition(QTextCursor::End);
	cursor.beginEditBlock();
	cursor.insertText(line + "\n");
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
