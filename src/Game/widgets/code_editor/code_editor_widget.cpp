#include "Game/widgets/code_editor/code_editor_widget.h"
#include "BaseWidgets/highlights/highlighter.h"

class CodeEditorWidget::Private {
public:
	Private(CodeEditorWidget* parent) : q(parent) {}
	CodeEditorWidget* q;

	void setupUI();
};

CodeEditorWidget::CodeEditorWidget(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->setupUI();
}

CodeEditorWidget::~CodeEditorWidget() = default;

void CodeEditorWidget::Private::setupUI() {

}
