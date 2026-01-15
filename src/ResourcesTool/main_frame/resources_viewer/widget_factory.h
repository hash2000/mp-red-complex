#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "ResourcesTool/main_frame/resources_viewer/widget_resources.h"
#include "Resources/resources.h"
#include <QVariantMap>
#include <QWidget>

class WidgetsFactory {
public:
	WidgetsFactory(std::shared_ptr<Resources> resources, const QVariantMap& params);

	BaseTabWidget* create(QWidget* parent = nullptr) const;

private:
	BaseTabWidget* createExternalWidget(const QString& type) const;
	BaseTabWidget* createResourceTypeWidget(WidgetResource type, QWidget* parent) const;

private:
	const QVariantMap& _params;
	std::shared_ptr<Resources> _resources;
};
