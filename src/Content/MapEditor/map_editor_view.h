#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include <QWidget>
#include <QVariantMap>

class MapEditorView : public BaseTabWidget {
public:
	explicit MapEditorView(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget *parent = nullptr);

	virtual ~MapEditorView() = default;

};
