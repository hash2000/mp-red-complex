#pragma once
#include "Launcher/main_frame/resources_viewer/widget_resources.h"
#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Game/data_format/int/procedure.h"
#include "Resources/resources.h"
#include <QVBoxLayout>
#include <memory>

class WidgetMaker: public QObject {
	Q_OBJECT
public:
	WidgetMaker(
		std::weak_ptr<StreamWidgetSelector> selector,
		std::shared_ptr<Resources> resources,
		QVBoxLayout *centerLayout,
		QVBoxLayout *actionsLayout);

	void make(WidgetResource type);

private slots:
	void onSelectProcedure(
		DataStream &stream,
		DataFormat::Int::Programmability &prog,
		DataFormat::Int::Procedure &proc);

private:
	void clearLayout(QVBoxLayout *layout);
	void makeHex(std::shared_ptr<DataStream> block);
	void makeText(std::shared_ptr<DataStream> block);
	void makeInt(std::shared_ptr<DataStream> block);
	void makeMsg(std::shared_ptr<DataStream> block);
	void makeSve(std::shared_ptr<DataStream> block);
	void makeBio(std::shared_ptr<DataStream> block);
	void makeGcd(std::shared_ptr<DataStream> block);
	void makeGam(std::shared_ptr<DataStream> block);

private:
	std::weak_ptr<StreamWidgetSelector> _selector;
	std::shared_ptr<Resources> _resources;
	QVBoxLayout *_centerLayout;
	QVBoxLayout *_actionsLayout;
};
