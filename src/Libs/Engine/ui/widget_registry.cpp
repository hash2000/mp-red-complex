#include "Libs/Engine/ui/widget_registry.h"
#include <QDebug>

class WidgetRegistry::Private {
public:
	std::map<QString, WidgetFactory> factory;
};

WidgetRegistry::WidgetRegistry() : d(std::make_unique<Private>()) {
}

WidgetRegistry::~WidgetRegistry() = default;

void WidgetRegistry::registerFactory(const QString& type, WidgetFactory factory) {
	if (d->factory.find(type) != d->factory.end()) {
		qWarning() << "WidgetRegistry: Widget type" << type << "allready registered";
		return;
	}

	d->factory.emplace(type, factory);
}

QWidget* WidgetRegistry::build(const QString& type, const WidgetTypeHandler& handler) {
	const auto& it = d->factory.find(type);
	if (it == d->factory.end()) {
		return nullptr;
	}

	return it->second(type, handler);
}

