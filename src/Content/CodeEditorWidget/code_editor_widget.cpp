#include "Content/CodeEditorWidget/code_editor_widget.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/highlighter.h"
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Libs/DataStream/file_reader.h"
#include "Libs/Base/extensions/text_edit_extensions.h"

#include <QThread>
#include <QFileInfo>
#include <QJsonParseError>
#include <QJsonDocument>

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;
	HighlightingPluginManager* pluginManager;
	FormatterPluginManager* formatterManager;
	QString path;
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

	const auto langType = plugin->languageInfo().mimeType;
	if (langType == "application/json") {
		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(toPlainText().toUtf8(), &error);
		if (error.error != QJsonParseError::NoError) {
			qWarning() << "JSON parse error:" << error.errorString();
			return;
		}

		setText(doc.toJson(QJsonDocument::Indented));
	}
	else if (langType == "text/html") {

	}
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

