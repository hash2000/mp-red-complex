#include "Game/widgets/code_editor/code_editor_widget.h"
#include "BaseWidgets/highlights/highlighter.h"
#include <QTextCharFormat>

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	Highlighter* highlighter = nullptr;

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
	numberFormat.setForeground(Qt::red);
	highlighter->addRule("numbers" ,"\\b[0-9]+\\b", numberFormat);

	QTextCharFormat stringFormat;
	stringFormat.setForeground(Qt::green);
	highlighter->addRule("strings", "\"[^\"]*\"", stringFormat);
}

void CodeEditorWidget::setPath(const QString& path) {

}
