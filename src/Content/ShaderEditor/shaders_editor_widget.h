#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "Resources/resources.h"
#include <QWidget>
#include <QVariantMap>

class ShaderControlPanel;

class ShadersEditorWidget : public BaseTabWidget {
public:
	explicit ShadersEditorWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* parent = nullptr);

	virtual ~ShadersEditorWidget();


private slots:
	void onRequestContainer(const QVariantMap& params);

private:
	ShaderControlPanel* _controlPanel;

};
