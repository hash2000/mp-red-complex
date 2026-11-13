#include "Libs/Engine/ui/widget_registry.h"
#include <QDebug>

class WidgetRegistry::Private {
public:
	std::map<QString, WidgetFactory> factoryByXtype;
	std::map<QString, WidgetFactory> factoryByClass;
};

WidgetRegistry::WidgetRegistry() : d(std::make_unique<Private>()) {
}

WidgetRegistry::~WidgetRegistry() = default;

void WidgetRegistry::registerFactory(const WidgetTypeHandler& type, WidgetFactory factory) {
	if (d->factoryByXtype.find(type.xtype) != d->factoryByXtype.end()) {
		qWarning() << "WidgetRegistry: Widget xtype" << type.xtype << "allready registered";
		return;
	}

	if (d->factoryByClass.find(type.className) != d->factoryByClass.end()) {
		qWarning() << "WidgetRegistry: Widget class" << type.className << "allready registered";
		return;
	}

	d->factoryByXtype.emplace(type.xtype, factory);
	d->factoryByClass.emplace(type.className, factory);
}

QWidget* WidgetRegistry::build(const QString& type, const WidgetBuildHandler& handler) {
	const auto& itxtype = d->factoryByXtype.find(type);
	if (itxtype != d->factoryByXtype.end()) {
		return itxtype->second(type, handler);
	}

	const auto& itclass = d->factoryByClass.find(type);
	if (itclass != d->factoryByClass.end()) {
		return itclass->second(type, handler);
	}

	return nullptr;
}

