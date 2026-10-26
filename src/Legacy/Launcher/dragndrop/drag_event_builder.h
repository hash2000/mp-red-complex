#pragma once
#include <QObject>

class ItemEntity;
class ItemMimeData;

class DragEventBuilder {
public:
	DragEventBuilder(QObject* parent, const ItemMimeData& itemMimeData, const ItemEntity& entity, const QUuid& inventoryId);
	~DragEventBuilder();

	Qt::DropAction ExecDrag(Qt::DropAction action);

private:
	class Private;
	std::unique_ptr<Private> d;
};
