#include "Launcher/main_frame/resources_viewer/widget_maker.h"
#include "Game/data_format/gcd/data_reader.h"
#include "Game/proto/gcd.h"
#include "Launcher/widgets/hex/hex_dump_widget.h"
#include "Launcher/widgets/hex/hex_control_panel.h"
#include "Launcher/widgets/procedure/procedure_explorer_widget.h"
#include "Launcher/widgets/messages/messages_explorer_widget.h"
#include "Launcher/widgets/pallete/pallete_explorer_widget.h"
#include "Game/data_format/int/data_reader.h"
#include "Game/data_format/int/code_reader.h"
#include "Game/data_format/txt/data_reader.h"
#include "Game/data_format/msg/data_reader.h"
#include "Game/data_format/sve/data_reader.h"
#include "Game/data_format/bio/data_reader.h"
#include "Game/data_format/gam/data_reader.h"
#include "Game/data_format/pro/data_reader.h"
#include "Game/data_format/frm/data_reader.h"
#include "Game/data_format/pal/data_reader.h"
#include <QWidget>
#include <QPlainTextEdit>

WidgetMaker::WidgetMaker(
	std::weak_ptr<StreamWidgetSelector> selector,
	std::shared_ptr<Resources> resources,
	QVBoxLayout *centerLayout,
	QVBoxLayout *actionsLayout)
: QObject()
, _selector(selector)
, _resources(resources)
, _centerLayout(centerLayout)
, _actionsLayout(actionsLayout) {
}

void WidgetMaker::make(WidgetResource type, const QString &suffix) {
	try {
		tryMake(type, suffix);
	} catch (std::exception &exc) {
		qDebug() << "Exception: " << exc.what();
	}
}

void WidgetMaker::tryMake(WidgetResource type, const QString &suffix) {
	auto sel = _selector.lock();
	if (!sel) {
		return;
	}

	auto streamOpt = sel->getStream(*_resources);
	if (!streamOpt) {
		return;
	}

	clearLayout(_centerLayout);
	clearLayout(_actionsLayout);

	auto stream = streamOpt.value();
	auto block = stream->makeBlockAsStream();

	switch(type) {
		case WidgetResource::Hex: makeHex(block); break;
    case WidgetResource::Text: makeText(block); break;
    case WidgetResource::Int: makeInt(block); break;
    case WidgetResource::Msg: makeMsg(block); break;
		case WidgetResource::Sve: makeSve(block); break;
		case WidgetResource::Bio: makeBio(block); break;
		case WidgetResource::Gcd: makeGcd(block); break;
		case WidgetResource::Gam: makeGam(block); break;
		case WidgetResource::Pro: makePro(block); break;
		case WidgetResource::Frm: makeFrm(block, suffix); break;
		case WidgetResource::Pal: makePal(block); break;
	}
}

void WidgetMaker::makePal(std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Pallete>();
	DataFormat::Pal::DataReader reader(*block);
	reader.read(*result);

	auto widget = new PalleteExplorerWidget();
	widget->setPalette(result->items);

	_centerLayout->addWidget(widget);
}

void WidgetMaker::makeFrm(std::shared_ptr<DataStream> block, const QString &suffix) {
	auto result = std::make_unique<Proto::Animation>();
	DataFormat::Frm::DataReader reader(*block);
	reader.read(*result, suffix);
}

void WidgetMaker::makePro(std::shared_ptr<DataStream> block) {
	DataFormat::Pro::DataReader reader(*block);
	auto result = reader.read();
}

void WidgetMaker::makeGam(std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::GlobalVariables>();
	DataFormat::Gam::DataReader reader(*block);
	reader.read(*result);
}

void WidgetMaker::makeGcd(std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Character>();
	DataFormat::Gcd::DataReader reader(*block);
	reader.read(*result);
}

void WidgetMaker::makeSve(std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::ScriptEntries>();
	DataFormat::Sve::DataReader reader(*block);
	reader.read(*result);

}

void WidgetMaker::makeBio(std::shared_ptr<DataStream> block) {
	QString result;
	DataFormat::Bio::DataReader reader(*block);
	reader.read(result);

	auto widget = new QPlainTextEdit;

	widget->setPlainText(result);
	_centerLayout->addWidget(widget);
}

void WidgetMaker::makeInt(std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Programmability>();
	DataFormat::Int::DataReader reader(*block);
	reader.read(*result);
	auto panel = new ProcedureExplorerWidget(std::move(result), block);
	_actionsLayout->addWidget(panel);

	connect(panel, &ProcedureExplorerWidget::selectProcedure,
		this, &WidgetMaker::onSelectProcedure);
}

void WidgetMaker::makeMsg(std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Messages>();
	DataFormat::Msg::DataReader reader(*block);
	reader.read(*result);

	auto panel = new MessagesExplorerWidget(std::move(result), block);
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

void WidgetMaker::onSelectProcedure(
	DataStream &stream,
	Proto::Programmability &prog,
	Proto::Procedure &proc)
{
	DataFormat::Int::CodeReader reader(stream, prog, proc);
	DataFormat::Int::Script script;
	reader.read(script);
}
