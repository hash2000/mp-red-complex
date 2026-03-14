#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include <QWidget>
#include <QVariantMap>

class ParticlesWidget : public BaseTabWidget {
public:
	explicit ParticlesWidget(Resources* resources, const QVariantMap& params,
		QWidget *parent = nullptr);

	virtual ~ParticlesWidget() = default;

};
