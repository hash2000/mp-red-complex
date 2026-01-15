#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include <QWidget>
#include <QVariantMap>

class ParticlesWidget : public BaseTabWidget {
public:
	explicit ParticlesWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget *parent = nullptr);

	virtual ~ParticlesWidget() = default;

};
