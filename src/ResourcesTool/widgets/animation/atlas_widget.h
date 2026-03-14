#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "Resources/resources.h"
#include <memory>

class AtlasWidget : public BaseTabWidget {
public:
	AtlasWidget(Resources* resources, const QVariantMap& params, QWidget* parent = nullptr);

};
