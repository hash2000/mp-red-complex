#pragma once
#include "Resources/resources.h"
#include <QStandardItem>
#include <memory>

class AssetsModelBuilderLinear {
public:
	AssetsModelBuilderLinear(QStandardItem* parent, const std::shared_ptr<Resources>& resources);
	void build();

private:
	QStandardItem* _parent;
	const std::shared_ptr<Resources> _resources;
};
