#pragma once
#include "Resources/resources.h"
#include <QWidget>
#include <QHBoxLayout>
#include <memory>

class BaseTabWidget : public QWidget {
public:
	BaseTabWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* parent = nullptr);

	std::shared_ptr<DataStream> currentStream() const;

private:
	std::shared_ptr<Resources> _resources;
	QVariantMap _params;
};


class BaseTabWidgetWrapper : public BaseTabWidget {
public:
	BaseTabWidgetWrapper(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* inner, QWidget* parent = nullptr)
	: BaseTabWidget(resources, params, parent) {
		auto layout = new QHBoxLayout(this);
		layout->addWidget(inner);
		setLayout(layout);
	}
};
