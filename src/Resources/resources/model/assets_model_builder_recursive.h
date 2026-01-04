#pragma once
#include "Resources/resources.h"
#include <QStandardItem>
#include <QMap>
#include <memory>

class AssetsModelBuilderRecursive {
public:
	AssetsModelBuilderRecursive(QStandardItem* parent, const DataStreamContainer& container);

	void build();

private:
	QStandardItem* createItemPart(QStandardItem *parent, const QString &name, const QString &fullPath);
	void buildItemsFromPath(QStandardItem *parent, const QString& fullPath);

private:
	QStandardItem* _parent;
	QMap<QString, QStandardItem*> _treeCache;
	const DataStreamContainer &_container;
};
