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
	QStandardItem* createFolder(const QString &name);

	void buildFromContainer(const DataStreamContainer& container);

	void buildItemsFromPath(const QString& fullPath);

private:
	QStandardItem* _parent;
	QMap<QString, QStandardItem*> _treeCache;
	const std::shared_ptr<Resources> _resources;
};
