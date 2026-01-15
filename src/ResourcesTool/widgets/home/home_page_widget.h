#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "Resources/resources.h"
#include "Engine/main_frame.h"
#include "BaseWidgets/layouts/FlowLayout.h"
#include <QWidget>
#include <QScrollArea>

class HomePageWidget : public BaseTabWidget {
	Q_OBJECT

public:
	HomePageWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		MainFrame *mainFrame, QWidget *parent = nullptr);

private:
	void setupButtons();
	void addButton(const QVariantMap& params);

signals:
	void requestTabCreation(const QVariantMap& params);

private:
	FlowLayout* _flowArea;
	QScrollArea* _scrollArea;
	MainFrame* _mainFrame;
};
