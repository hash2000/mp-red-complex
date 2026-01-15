#include "ResourcesTool/widgets/text/text_widget.h"
#include <QPlainTextEdit>

TextWidget::TextWidget(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget* parent)
: BaseTabWidget(resources, params, parent) {

	auto block = currentStream();
	auto byteBlock = block->readBlockAsQByteArray();
	auto view = new QPlainTextEdit;
	view->setPlainText(byteBlock);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(view);
	setLayout(layout);
}
