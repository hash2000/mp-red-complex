#pragma once
#include "Launcher/main_frame/resources_viewer/widget_resources.h"
#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Resources/resources.h"
#include <QVBoxLayout>
#include <memory>

class WidgetMaker {
public:
	WidgetMaker(StreamWidgetSelector &selector,
		WidgetResource type,
		std::shared_ptr<Resources> resources,
		QVBoxLayout *centerLayout,
		QVBoxLayout *actionsLayout);

	void make();

private:
	void clearLayout(QVBoxLayout *layout);
	void makeHex(std::shared_ptr<DataStream> block);
	void makeText(std::shared_ptr<DataStream> block);
	void makeInt(std::shared_ptr<DataStream> block);

private:
	StreamWidgetSelector &_selector;
	WidgetResource _type;
	std::shared_ptr<Resources> _resources;
	QVBoxLayout *_centerLayout;
	QVBoxLayout *_actionsLayout;
};
