#include "ApplicationLayer/items_save_manager.h"
#include "ApplicationLayer/items/items_service.h"
#include "DataLayer/items/i_items_data_writer.h"
#include "DataLayer/items/item.h"
#include <QDebug>

class ItemsSaveManager::Private {
public:
	Private(ItemsSaveManager* parent)
		: q(parent) {
	}

	ItemsSaveManager* q;
	ItemsService* itemsService = nullptr;
	IItemsDataWriter* itemsDataWriter = nullptr;

	bool saveAll() {
		if (!itemsService || !itemsDataWriter) {
			return false;
		}

		bool success = true;

		for (const auto& item : itemsService->items()) {
			if (!itemsDataWriter->saveItem(item.id, item)) {
				qWarning() << "ItemsSaveManager: failed to save item" << item.id;
				success = false;
			}
			else {
				qDebug() << "ItemsSaveManager: saved item" << item.id;
			}
		}

		return success;
	}
};

ItemsSaveManager::ItemsSaveManager(
	ItemsService* itemsService,
	IItemsDataWriter* itemsDataWriter,
	QObject* parent)
: QObject(parent)
, d(std::make_unique<Private>(this)) {
	d->itemsService = itemsService;
	d->itemsDataWriter = itemsDataWriter;
}

ItemsSaveManager::~ItemsSaveManager() = default;

void ItemsSaveManager::saveAll() {
	d->saveAll();
}
