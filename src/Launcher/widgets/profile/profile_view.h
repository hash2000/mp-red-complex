#pragma once
#include "Launcher/widgets/side_bar/side_bar_action.h"
#include <QWidget>

class ProfileView : public QWidget,  public SideBarAction {
public:
	ProfileView(QWidget* parent = nullptr);

	void onAction() override;
	virtual QIcon icon() const override;
	virtual QString description() const override;
	QString id() const override;
	ViewState viewState() const override;
};
