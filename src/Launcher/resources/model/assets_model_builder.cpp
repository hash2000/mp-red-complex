#include "Launcher/resources/model/assets_model_builder.h"
#include "Launcher/resources/model/assets_model.h"

AssetsModelBuilder::AssetsModelBuilder(QStandardItem *parent, const std::shared_ptr<Resources>& resources)
	: _parent(parent)
	, _resources(resources) {
}

void AssetsModelBuilder::build() {
	for(const auto &res : _resources->items()) {
		buildFromContainer(res);
	}
}

QStandardItem *AssetsModelBuilder::createItemPart(QStandardItem *parent, const QString &name, const QString &fullPath) {
	auto part = new QStandardItem(name);

	QFileInfo info(name);
	const auto& suffix = info.suffix();

	part->setData(parent->data(static_cast<int>(AssetsViewItemRole::ContainerName)), static_cast<int>(AssetsViewItemRole::ContainerName));
	part->setData(fullPath, static_cast<int>(AssetsViewItemRole::FullPath));
	part->setData(suffix, static_cast<int>(AssetsViewItemRole::Suffix));

	if (suffix.isEmpty()) {
		part->setData(QVariant::fromValue(AssetsViewItemType::Folder), static_cast<int>(AssetsViewItemRole::Type));
		part->setIcon(QIcon::fromTheme("folder"));
	} else {
		part->setData(QVariant::fromValue(AssetsViewItemType::File), static_cast<int>(AssetsViewItemRole::Type));
		part->setIcon(QIcon::fromTheme("text-x-generic"));
	}

	return part;
}

void AssetsModelBuilder::buildFromContainer(const DataStreamContainer &container) {
	_treeCache.clear();
	auto containerItem = new QStandardItem(container.name());
	containerItem->setData(container.name(), static_cast<int>(AssetsViewItemRole::ContainerName));
	containerItem->setData((unsigned char)AssetsViewItemType::Container, static_cast<int>(AssetsViewItemRole::Type));
	containerItem->setIcon(QIcon::fromTheme("text-x-generic"));
	_parent->appendRow(containerItem);

	for(const auto &item : container.items()) {
		const auto &name = item.name();
		buildItemsFromPath(containerItem, name);
	}
}

void AssetsModelBuilder::buildItemsFromPath(QStandardItem *parent, const QString& fullPath) {
	const auto &cleanPath = QDir::cleanPath(fullPath);
	if (cleanPath.isEmpty()) {
		return;
	}

	const auto &parts = cleanPath.split('\\', Qt::SkipEmptyParts);
	QString currentPath;
	QStandardItem* parentItem = parent;

	for (qsizetype i = 0, count = parts.size(); i < count; i++) {
		const auto part = parts[i];
		if (i == 0) {
			currentPath = part;
		}
		else {
			currentPath += "/" + part;
		}

		if (!_treeCache.contains(currentPath)) {
      auto item = createItemPart(parent, part, cleanPath);
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
