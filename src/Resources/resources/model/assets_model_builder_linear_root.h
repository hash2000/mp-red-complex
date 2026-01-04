#pragma once
#include "Resources/resources.h"
#include <QStandardItem>
#include <memory>

class AssetsModelBuilderLinearRoot {
public:
	AssetsModelBuilderLinearRoot(QStandardItem* parent, const std::shared_ptr<Resources>& resources);

	void build();

private:
	void buildFromContainer(const DataStreamContainer& container);

private:
	QStandardItem* _parent;
	const std::shared_ptr<Resources> _resources;
};
