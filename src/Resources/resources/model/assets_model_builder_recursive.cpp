#include "Resources/resources/model/assets_model_builder_recursive.h"
#include "Resources/resources/model/assets_model.h"
#include "Base/scoped_timer.h"

AssetsModelBuilderRecursive::AssetsModelBuilderRecursive(QStandardItem *parent,
	const std::shared_ptr<Resources>& resources)
	: _parent(parent)
	, _resources(resources) {
}

void AssetsModelBuilderRecursive::build() {
	for(const auto &res : _resources->items()) {
		ScopedTimer watch(QString("Build files tree for resource %1").arg(res.name()));
		buildFromContainer(res);
	}
}

QStandardItem *AssetsModelBuilderRecursive::createItemPart(QStandardItem *parent,
	const QString &name, const QString &fullPath) {
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
		part->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew));
	}

	return part;
}

void AssetsModelBuilderRecursive::buildFromContainer(const DataStreamContainer &container) {
	_treeCache.clear();

	QString suffix = "";
	switch (container.type()) {
	case ContainerType::Repository_v1:
		suffix = "*";
		break;
	case ContainerType::Directory:
		suffix = "[_]";
		break;
	case ContainerType::Undefined:
		break;
	}

	const auto name = QString("%1 %2")
		.arg(container.name())
		.arg(suffix);

	auto containerItem = new QStandardItem(name);
	containerItem->setData(container.name(), static_cast<int>(AssetsViewItemRole::ContainerName));
	containerItem->setData((unsigned char)AssetsViewItemType::Container, static_cast<int>(AssetsViewItemRole::Type));
	containerItem->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DriveHarddisk));
	_parent->appendRow(containerItem);

	for(const auto &item : container.items()) {
		const auto &name = item.name();
		buildItemsFromPath(containerItem, name);
	}
}

void AssetsModelBuilderRecursive::buildItemsFromPath(QStandardItem *parent, const QString& fullPath) {
	const auto &cleanPath = QDir::cleanPath(fullPath);
	if (cleanPath.isEmpty()) {
		return;
	}

	const auto &parts = cleanPath.split('/', Qt::SkipEmptyParts);
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
