#include "Game/widgets/code_editor/code_editor_widget.h"
#include "BaseWidgets/highlights/highlighter.h"
#include "DataStream/file_reader.h"

#include <QThread>

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;
	QString path;

	void setupUI();
};

CodeEditorWidget::CodeEditorWidget(QWidget* parent)
	: QTextEdit(parent)
	, d(std::make_unique<Private>(this)) {
	d->setupUI();
}

CodeEditorWidget::~CodeEditorWidget() = default;

void CodeEditorWidget::Private::setupUI() {
	highlighter = new Highlighter(q->document());

	QTextCharFormat numberFormat;
	numberFormat.setForeground(Qt::green);
	highlighter->addRule("numbers" ,"\\b[0-9]+\\b", numberFormat);

	QTextCharFormat stringFormat;
	stringFormat.setForeground(Qt::green);
	highlighter->addRule("strings", "\"[^\"]*\"", stringFormat);
}

void CodeEditorWidget::setPath(const QString& path) {
	d->path = path;
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
