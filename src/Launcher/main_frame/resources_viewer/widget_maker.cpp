#include "Launcher/main_frame/resources_viewer/widget_maker.h"
#include "Base/format.h"
#include "DataFormat/data_format/gcd/data_reader.h"
#include "DataFormat/proto/gcd.h"
#include "Launcher/widgets/hex/hex_dump_widget.h"
#include "Launcher/widgets/hex/hex_control_panel.h"
#include "Launcher/widgets/procedure/procedure_explorer_widget.h"
#include "Launcher/widgets/messages/messages_explorer_widget.h"
#include "Launcher/widgets/pallete/pallete_explorer_widget.h"
#include "Launcher/widgets/animation/frames_explorer_widget.h"
#include "DataFormat/data_format/int/data_reader.h"
#include "DataFormat/data_format/int/code_reader.h"
#include "DataFormat/data_format/txt/data_reader.h"
#include "DataFormat/data_format/msg/data_reader.h"
#include "DataFormat/data_format/sve/data_reader.h"
#include "DataFormat/data_format/bio/data_reader.h"
#include "DataFormat/data_format/gam/data_reader.h"
#include "DataFormat/data_format/pro/data_reader.h"
#include "DataFormat/data_format/frm/data_reader.h"
#include "DataFormat/data_format/pal/data_reader.h"
#include <QWidget>
#include <QSplitter>
#include <QPlainTextEdit>

WidgetMaker::WidgetMaker(
	std::weak_ptr<StreamWidgetSelector> selector,
	std::shared_ptr<Resources> resources,
	QTabWidget *centerTabs)
: QObject()
, _selector(selector)
, _resources(resources)
, _centerTabs(centerTabs) {
}

void WidgetMaker::make(WidgetResource type, const QString &suffix) {
	try {
		tryMake(type, suffix);
	} catch (std::exception &exc) {
		qDebug() << "Exception: " << exc.what();
	}
}

bool WidgetMaker::selectTabByName(const QString &name) {
	for (int i = 0; i < _centerTabs->count(); i++) {
		const auto widget = _centerTabs->widget(i);
		if (widget && widget->property("tabId") == name) {
			_centerTabs->setCurrentIndex(i);
			return true;
		}
	}
	return false;
}

void WidgetMaker::addWidgetTab(WidgetResource type, std::shared_ptr<DataStream> block, QWidget *widget, QWidget *actionsPanel) {
	const auto typeName = Format<WidgetResource>::format(type);
	const auto tabName = QString("%1://%2")
		.arg(typeName)
		.arg(block->fullPath());

	if (selectTabByName(tabName)) {
		return;
	}

	if (actionsPanel) {
		auto splitter = new QSplitter(Qt::Horizontal);
		splitter->addWidget(actionsPanel);
		splitter->addWidget(widget);
		splitter->setStretchFactor(0, 0);
		splitter->setStretchFactor(1, 1);
		widget = splitter;
	}

	widget->setProperty("tabId", tabName);

	const auto index = _centerTabs->addTab(widget, block->name());
	_centerTabs->setCurrentIndex(index);
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

	auto stream = streamOpt.value();
	auto block = stream->makeBlockAsStream();

	switch(type) {
		case WidgetResource::Hex: makeHex(type, block); break;
    case WidgetResource::Text: makeText(type, block); break;
    case WidgetResource::Int: makeInt(type, block); break;
    case WidgetResource::Msg: makeMsg(type, block); break;
		case WidgetResource::Sve: makeSve(type, block); break;
		case WidgetResource::Bio: makeBio(type, block); break;
		case WidgetResource::Gcd: makeGcd(type, block); break;
		case WidgetResource::Gam: makeGam(type, block); break;
		case WidgetResource::Pro: makePro(type, block); break;
		case WidgetResource::Frm: makeFrm(type, block, suffix); break;
		case WidgetResource::Pal: makePal(type, block); break;
	}
}

void WidgetMaker::makePal(WidgetResource type, std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Pallete>();
	DataFormat::Pal::DataReader reader(*block);
	reader.read(*result);

	auto widget = new PalleteExplorerWidget();
	widget->setPalette(result->items);
	addWidgetTab(type, block, widget);
}

void WidgetMaker::makeFrm(WidgetResource type, std::shared_ptr<DataStream> block, const QString &suffix) {
	auto result = std::make_unique<Proto::Animation>();
	DataFormat::Frm::DataReader reader(*block);
	reader.read(*result, suffix);

	auto palleteBlockOpt = _resources->getStream("master.dat", "color.pal");
	auto palleteBlock = palleteBlockOpt.value()->makeBlockAsStream();
	auto pallete = std::make_unique<Proto::Pallete>();
	DataFormat::Pal::DataReader pallete_reader(*palleteBlock);
	pallete_reader.read(*pallete);

	auto widget = new FramesExplorerWidget;
	widget->setup(*result, *pallete);
	addWidgetTab(type, block, widget);
}

void WidgetMaker::makePro(WidgetResource type, std::shared_ptr<DataStream> block) {
	DataFormat::Pro::DataReader reader(*block);
	auto result = reader.read();
}

void WidgetMaker::makeGam(WidgetResource type, std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::GlobalVariables>();
	DataFormat::Gam::DataReader reader(*block);
	reader.read(*result);
}

void WidgetMaker::makeGcd(WidgetResource type, std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Character>();
	DataFormat::Gcd::DataReader reader(*block);
	reader.read(*result);
}

void WidgetMaker::makeSve(WidgetResource type, std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::ScriptEntries>();
	DataFormat::Sve::DataReader reader(*block);
	reader.read(*result);

}

void WidgetMaker::makeBio(WidgetResource type, std::shared_ptr<DataStream> block) {
	QString result;
	DataFormat::Bio::DataReader reader(*block);
	reader.read(result);

	auto widget = new QPlainTextEdit;

	widget->setPlainText(result);
	addWidgetTab(type, block, widget);
}

void WidgetMaker::makeInt(WidgetResource type, std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Programmability>();
	DataFormat::Int::DataReader reader(*block);
	reader.read(*result);

	auto widget = new ProcedureExplorerWidget(std::move(result), block);
	connect(widget, &ProcedureExplorerWidget::selectProcedure,
		this, &WidgetMaker::onSelectProcedure);

	addWidgetTab(type, block, widget);
}

void WidgetMaker::makeMsg(WidgetResource type, std::shared_ptr<DataStream> block) {
	auto result = std::make_unique<Proto::Messages>();
	DataFormat::Msg::DataReader reader(*block);
	reader.read(*result);

	auto widget = new MessagesExplorerWidget(std::move(result), block);

	addWidgetTab(type, block, widget);
}

void WidgetMaker::makeText(WidgetResource type, std::shared_ptr<DataStream> block) {
	QString result;
	DataFormat::Txt::DataReader reader( *block);
	reader.read(result);

	auto widget = new QPlainTextEdit;

	widget->setPlainText(result);
	addWidgetTab(type, block, widget);
}

void WidgetMaker::makeHex(WidgetResource type, std::shared_ptr<DataStream> block) {
	auto byteBlock = block->readBlockAsQByteArray();
	auto widget = new HexDumpWidget;
	widget->setData(byteBlock);

	auto panel = new HexControlPanel(widget, block);

	addWidgetTab(type, block, widget, panel);
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
