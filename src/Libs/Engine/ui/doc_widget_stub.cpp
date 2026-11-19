#include "Libs/Engine/ui/doc_widget_stub.h"

#include <QVBoxLayout>

DocumentWidgetStub::DocumentWidgetStub(QWidget* content, QWidget* parent)
	: DocumentWidget(parent)
	, _content(content) {
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	if (content) {
		// addWidget переводит содержимое во владение обёртки.
		layout->addWidget(content);
	}
}

DocumentWidgetStub::~DocumentWidgetStub() = default;

bool DocumentWidgetStub::maybeClose() {
	return DocumentWidget::maybeClose() && (!_content || _content->close());
}
