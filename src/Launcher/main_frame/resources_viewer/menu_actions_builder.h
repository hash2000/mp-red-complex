#pragma once
#include "Resources/resources/model/assets_model.h"
#include <QMenu>
#include <memory>

class MenuActionsBuilder {
public:
	MenuActionsBuilder(QWidget *parent);

	std::unique_ptr<QMenu> Build(AssetsViewItemType type, const QString &suffix);

private:
	QWidget *_parent;
};
