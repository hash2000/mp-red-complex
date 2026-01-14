#pragma once
#include "ResourcesTool/main_frame/resources_viewer/stream_widget_selector.h"
#include "Resources/resources/model/assets_model.h"
#include <QMenu>
#include <memory>

class MenuActionsBuilder {
public:
	MenuActionsBuilder(QWidget *parent, std::shared_ptr<StreamWidgetSelector> selector);

	std::unique_ptr<QMenu> Build();

private:
	QWidget *_parent;
	std::shared_ptr<StreamWidgetSelector> _selector;
};
