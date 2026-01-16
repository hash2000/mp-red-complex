#include "ResourcesTool/widgets/text/text_widget.h"
#include "DataFormat/data_format/txt/data_reader.h"
#include <QPlainTextEdit>

TextWidget::TextWidget(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget* parent)
: BaseTabWidget(resources, params, parent) {
	auto block = currentStream();
	auto view = new QPlainTextEdit;

	QString value;
	DataFormat::Txt::DataReader reader(*block);
	reader.read(value);

	view->setPlainText(value);

	auto layout = new QHBoxLayout(this);
	layout->addWidget(view);
	setLayout(layout);
}
