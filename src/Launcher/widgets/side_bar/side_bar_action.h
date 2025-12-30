#pragma once
#include "Launcher/view_state.h"
#include <QWidget>

class SideBarAction {
public:
	virtual void onAction() = 0;
	virtual QString id() const = 0;
	virtual QIcon icon() const = 0;
	virtual QString description() const = 0;
	virtual ViewState viewState() const = 0;

	virtual bool isVisible() const {
		return true;
	}

	virtual bool isEnabled() const {
		return true;
	}
};
