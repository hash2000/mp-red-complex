#pragma once
#include "Launcher/resources.h"
#include <QStandardItem>
#include <QMap>
#include <memory>

class AssetsModelBuilder {
public:
	AssetsModelBuilder(QStandardItem* parent, const std::shared_ptr<Resources>& resources);

	void build();

private:
	QStandardItem* createItemPart(QStandardItem *parent, const QString &name, const QString &fullPath);

	void buildFromContainer(const DataStreamContainer& container);

	void buildItemsFromPath(QStandardItem *parent, const QString& fullPath);

private:
	QStandardItem* _parent;
	QMap<QString, QStandardItem*> _treeCache;
	const std::shared_ptr<Resources> _resources;
};
