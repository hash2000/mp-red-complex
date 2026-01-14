#pragma once
#include "ResourcesTool/widgets/base_tab_widget.h"
#include <QVariantMap>
#include <QWidget>

class WidgetsFactory {
public:
	WidgetsFactory(const QVariantMap& params);

	BaseTabWidget* create(QWidget* parent = nullptr) const;

private:
	QWidget* createBaseWidget() const;

private:
	const QVariantMap& _params;
};
