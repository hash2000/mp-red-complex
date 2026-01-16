#include "ResourcesTool/main_frame/resources_viewer/resources_tree.h"
#include "Resources/resources/model/assets_model.h"
#include <QJsonDocument>
#include <QDrag>

ResourcesTree::ResourcesTree(QWidget* parent)
: QTreeView(parent) {
	setDragEnabled(true);
	setAcceptDrops(false);
	setDragDropMode(QAbstractItemView::DragOnly);
}

void ResourcesTree::startDrag(Qt::DropActions supportedActions) {
	const QModelIndexList indexes = selectedIndexes();
	if (indexes.isEmpty()) {
		return;
	}

	QVariantMap dataMap;

	for (const auto& index : indexes) {
		if (index.column() != 0) {
			continue;
		}

		const auto model = index.model();
		if (!model) {
			continue;
		}

		const auto container = index.data(static_cast<int>(AssetsViewItemRole::ContainerName)).toString();
		const auto path = index.data(static_cast<int>(AssetsViewItemRole::FullPath)).toString();
		const auto suffix = index.data(static_cast<int>(AssetsViewItemRole::Suffix)).toString();
		const auto type = index.data(static_cast<int>(AssetsViewItemRole::Type)).value<AssetsViewItemType>();

		dataMap["container.name"] = container;
		dataMap["container.path"] = path;
		dataMap["suffix"] = path;
		dataMap["asset.type"] = (unsigned char)type;
		dataMap["action"] = "drop";
		break;
	}

	if (dataMap.isEmpty()) {
		return;
	}

	QMimeData* mimeData = new QMimeData();
	QByteArray jsonData = QJsonDocument::fromVariant(dataMap).toJson();
	mimeData->setData("application/x-container", jsonData);

	QDrag* drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->exec(Qt::CopyAction);
}
