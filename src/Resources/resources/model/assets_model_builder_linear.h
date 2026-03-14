#pragma once
#include "Resources/resources.h"
#include <QStandardItem>
#include <memory>

class AssetsModelBuilderLinear {
public:
	AssetsModelBuilderLinear(QStandardItem* parent, const Resources* resources);
	void build();

private:
	QStandardItem* _parent;
	const Resources* _resources;
};
