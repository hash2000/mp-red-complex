#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include <QWidget>
#include <QVariantMap>

class MapEditorWidget : public BaseTabWidget {
public:
	explicit MapEditorWidget(Resources* resources, const QVariantMap& params,
		QWidget *parent = nullptr);

	virtual ~MapEditorWidget() = default;

};
