#include "Launcher/main_frame/resources_viewer/widget_maker.h"
#include "Launcher/widgets/hex/hex_dump_widget.h"
#include "Launcher/widgets/hex/hex_control_panel.h"
#include "Launcher/widgets/procedure/procedure_explorer_widget.h"
#include "Game/data_format/int/data_reader.h"
#include "Game/data_format/txt/data_reader.h"
#include <QWidget>
#include <QPlainTextEdit>

WidgetMaker::WidgetMaker(StreamWidgetSelector &selector,
	WidgetResource type,
	std::shared_ptr<Resources> resources,
	QVBoxLayout *centerLayout,
	QVBoxLayout *actionsLayout)
: _selector(selector)
, _type(type)
, _resources(resources)
, _centerLayout(centerLayout)
, _actionsLayout(actionsLayout) {

}

void WidgetMaker::make() {
	auto streamOpt = _selector.getStream(*_resources);
	if (!streamOpt) {
		return;
	}

	clearLayout(_centerLayout);
	clearLayout(_actionsLayout);

	auto stream = streamOpt.value();
	auto block = stream->makeBlockAsStream();

	switch(_type) {
		case WidgetResource::Hex:
			makeHex(block);
			break;
    case WidgetResource::Text:
			makeText(block);
      break;
    case WidgetResource::Int:
			makeInt(block);
      break;
    }
}

void WidgetMaker::makeInt(std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<DataFormat::Int::Programmability>();
	DataFormat::Int::DataReader reader(*block);
	reader.read(*result);
	auto panel = new ProcedureExplorerWidget(std::move(result));
	_actionsLayout->addWidget(panel);
}

void WidgetMaker::makeText(std::shared_ptr<DataStream> block) {
	QString result;
	DataFormat::Txt::DataReader reader( *block);
	reader.read(result);

	auto widget = new QPlainTextEdit;

	widget->setPlainText(result);
	_centerLayout->addWidget(widget);
}

void WidgetMaker::makeHex(std::shared_ptr<DataStream> block) {
	auto byteBlock = block->readBlockAsQByteArray();
	auto widget = new HexDumpWidget;

	widget->setData(byteBlock);
	_centerLayout->addWidget(widget);

	auto panel = new HexControlPanel(widget, block);
	_actionsLayout->addWidget(panel);
}

void WidgetMaker::clearLayout(QVBoxLayout *layout) {
  QLayoutItem *item;
  while ((item = layout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->setParent(nullptr);
      delete item->widget();
    }
    delete item;
  }
}
