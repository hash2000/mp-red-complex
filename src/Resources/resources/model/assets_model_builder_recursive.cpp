#include "Resources/resources/model/assets_model_builder_recursive.h"
#include "Resources/resources/model/assets_model.h"
#include "Base/scoped_timer.h"

AssetsModelBuilderRecursive::AssetsModelBuilderRecursive(QStandardItem *parent, const DataStreamContainer& container)
	: _parent(parent)
	, _container(container) {
}

void AssetsModelBuilderRecursive::build() {
	ScopedTimer watch(QString("Build files tree for resource %1")
		.arg(_container.name()));

	_treeCache.clear();

	for (const auto& item : _container.items()) {
		const auto& name = item.name();
		buildItemsFromPath(_parent, name);
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
