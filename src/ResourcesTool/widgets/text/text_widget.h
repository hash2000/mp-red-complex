#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "Resources/resources.h"
#include <QWidget>

class TextWidget : public BaseTabWidget {
public:
	explicit TextWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* parent = nullptr);
	~TextWidget() override = default;
};
