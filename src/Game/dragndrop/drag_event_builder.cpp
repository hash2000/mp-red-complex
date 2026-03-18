#include "Game/dragndrop/drag_event_builder.h"
#include "Game/styles/items_styles.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include "DataLayer/items/item.h"
#include <QDrag>
#include <QMimeData>

class DragEventBuilder::Private {
public:
	Private(DragEventBuilder* parent)
		: q(parent) {
	}

	DragEventBuilder* q;
	QDrag* drag;
};

DragEventBuilder::DragEventBuilder(QObject* parent, const ItemMimeData& itemMimeData, const ItemEntity& entity, const QUuid& inventoryId)
: d(std::make_unique<Private>(this)) {
	d->drag = new QDrag(parent);
	auto mimeData = new QMimeData();

	mimeData->setData("application/x-game-item", itemMimeData.toMimeData());
	mimeData->setData("application/x-game-item-source-inventory-id", inventoryId.toString().toUtf8());
	mimeData->setText(entity.name);

	// Иконка для курсора
	QPixmap dragPixmap = entity.icon.scaled(
		ItemsStyles::CELL_SIZE,
		ItemsStyles::CELL_SIZE,
		Qt::KeepAspectRatio, Qt::SmoothTransformation);
	d->drag->setMimeData(mimeData);
	d->drag->setPixmap(dragPixmap);
	d->drag->setHotSpot(dragPixmap.rect().center());
}

DragEventBuilder::~DragEventBuilder() = default;

Qt::DropAction DragEventBuilder::ExecDrag(Qt::DropAction action) {
	return d->drag->exec(action);
}
