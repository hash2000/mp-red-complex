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
	void setupHighlighterRules(const QString& language);
	void setupStyling();
};

CodeEditorWidget::CodeEditorWidget(HighlightingPluginManager* pluginManager, QWidget* parent)
	: QTextEdit(parent)
	, d(std::make_unique<Private>(this)) {
	d->pluginManager = pluginManager;
	d->setupUI();
	d->setupStyling();

	connect(d->highlighter, &Highlighter::requestEmbeddedLanguage, this, &CodeEditorWidget::onRequestEmbeddedLanguage);
	connect(d->highlighter, &Highlighter::requestEmbeddedBlockEnd, this, &CodeEditorWidget::onRequestEmbeddedBlockEnd);
}

CodeEditorWidget::~CodeEditorWidget() = default;

void CodeEditorWidget::Private::setupUI() {
	highlighter = new Highlighter(q->document());
	setupHighlighterRules("txt");
}

void CodeEditorWidget::Private::setupHighlighterRules(const QString& language) {
	if (!pluginManager) {
		return;
	}

	highlighter->lockRefreshView();

	if (lastPlugin) {
		lastPlugin->uninstall(*highlighter);
	}

	const auto plugin = pluginManager->pluginForLanguage(language);
	if (!plugin) {
		highlighter->lockRefreshView(false);
		return;
	}

	plugin->install(*highlighter, HighlighterRuleType::Global);
	lastPlugin = plugin;
	highlighter->lockRefreshView(false);
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
	d->setupHighlighterRules(info.suffix().toLower());
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

void CodeEditorWidget::onRequestEmbeddedLanguage(const QString& languageId) {
	d->highlighter->clearEmbeddedRules();
	const auto plugin = d->pluginManager->pluginForLanguage(languageId);
	if (!plugin) {
		return;
	}

	plugin->install(*d->highlighter, HighlighterRuleType::Embedded);
}

void CodeEditorWidget::onRequestEmbeddedBlockEnd() {
	d->highlighter->clearEmbeddedRules();
}
