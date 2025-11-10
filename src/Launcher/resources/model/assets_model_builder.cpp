#include "Launcher/resources/model/assets_model_builder.h"

AssetsModelBuilder::AssetsModelBuilder(QStandardItem *parent, const std::shared_ptr<Resources>& resources)
	: _parent(parent)
	, _resources(resources) {
}

void AssetsModelBuilder::build() {
	_treeCache.clear();
	for(const auto &res : _resources->items()) {
		buildFromContainer(res);
	}
}

QStandardItem *AssetsModelBuilder::createFolder(const QString &name) {
	auto folder = new QStandardItem(name);
	folder->setData("folder", Qt::UserRole);
	folder->setIcon(QIcon(":/icons/folder.png"));
	return folder;
}

void AssetsModelBuilder::buildFromContainer(const DataStreamContainer &container) {
	for(const auto &item : container.items()) {
		const auto name = item.name();
		buildItemsFromPath(name);
	}
}

void AssetsModelBuilder::buildItemsFromPath(const QString& fullPath) {
	const auto cleanPath = QDir::cleanPath(fullPath);
	if (cleanPath.isEmpty()) {
		return;
	}

	const auto parts = cleanPath.split('\\', Qt::SkipEmptyParts);
	QString currentPath;
	QStandardItem* parentItem = _parent;

	for (qsizetype i = 0, count = parts.size(); i < count; i++) {
		const auto part = parts[i];
		if (i == 0) {
			currentPath = part;
		}
		else {
			currentPath += "/" + part;
		}

		if (!_treeCache.contains(currentPath)) {
      auto item = new QStandardItem(part);
			item->setData("");
      _treeCache[currentPath] = item;

      if (parentItem) {
        parentItem->appendRow(item);
      }

      parentItem = item;
    } else {
      parentItem = _treeCache[currentPath];
		}
	}
}
