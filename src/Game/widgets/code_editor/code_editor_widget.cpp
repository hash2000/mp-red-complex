#include "Game/widgets/code_editor/code_editor_widget.h"
#include "BaseWidgets/highlights/highlighter.h"
#include "BaseWidgets/highlights/rules/highlighter_rule_default.h"
#include "BaseWidgets/highlights/rules/highlighter_rule_glsl.h"
#include "DataStream/file_reader.h"

#include <QThread>
#include <QFileInfo>

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;
	QString path;
	QString previusSuffix;

	void setupUI();
	void setupHighlighterRules();
};

CodeEditorWidget::CodeEditorWidget(QWidget* parent)
	: QTextEdit(parent)
	, d(std::make_unique<Private>(this)) {
	d->setupUI();
}

CodeEditorWidget::~CodeEditorWidget() = default;

void CodeEditorWidget::Private::setupUI() {
	highlighter = new Highlighter(q->document());
	setupHighlighterRules();
}

void CodeEditorWidget::Private::setupHighlighterRules() {
	QFileInfo file(path);
	const auto suffix = file.suffix();

	if (suffix.isEmpty()) {
		highlighter::rule::def::apply(*highlighter);
		return;
	}

	if (previusSuffix == "vert" || previusSuffix == "frag") {
		if (suffix == "vert" || suffix == "frag") {
			return;
		}
		highlighter->clearRules();
	}

	previusSuffix = suffix;

	if (suffix == "vert" || suffix == "frag") {
		highlighter::rule::glsl::apply(*highlighter);
	}
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
