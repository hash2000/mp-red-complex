#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventories_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/inventory/inventory_item_mime_data.h"
#include "ApplicationLayer/items/items_service.h"
#include <QHash>
#include <QDebug>
#include <vector>

struct InventoryViewCell {
	bool occupied = false;
	InventoryItemHandler* item = nullptr;
};


class InventoryService::Private {
public:
	Private(InventoryService* parent)
	: q(parent) {
	}

	InventoryItemHandler* duplicateItem(const QString& id) {
		auto newItemDuplicate = itemsService->duplicate(id, ItemOwner::Inventory);
		return makeInventoryitem(newItemDuplicate);
	}

	InventoryItemHandler* makeInventoryitem(const Item* item) {
		if (!item) {
			return nullptr;
		}

		auto newItemPtr = std::make_unique<InventoryItemHandler>();
		auto result = newItemPtr.get();
		newItemPtr->entity = item->entity;
		newItemPtr->id = item->id;
		newItemPtr->count = 0;
		newItemPtr->x = 0;
		newItemPtr->y = 0;

		inventoryItems.emplace(newItemPtr->id, std::move(newItemPtr));
		return result;
	}

	void setupCells() {
		if (inventoryItems.empty()) {
			cells.clear();
		}

		cells.resize(cols);
		for (int col = 0; col < cols; col++) {
			cells[col].resize(rows);
			for (int row = 0; row < rows; row++) {
				auto cell = std::make_unique<InventoryViewCell>();
				cells[col][row] = std::move(cell);
			}
		}

		items.clear();

		for (const auto& itemIt : inventoryItems) {
			auto item = itemIt.second.get();
			if (!item) {
				continue;
			}

			// Проверка валидности координат предмета
			if (item->x < 0 || item->y < 0 ||
				item->x + item->entity->width > cols ||
				item->y + item->entity->height > rows) {
				qWarning() << "InventoryService: item" << item->id
					<< "has invalid position (" << item->x << "," << item->y
					<< ") for grid" << cols << "x" << rows;
				continue;
			}

			// Занимаем ячейки предметом
			bool occupiedConflict = false;
			for (int dy = 0; dy < item->entity->height && !occupiedConflict; dy++) {
				for (int dx = 0; dx < item->entity->width && !occupiedConflict; dx++) {
					int cellCol = item->x + dx;
					int cellRow = item->y + dy;

					if (cells[cellCol][cellRow]->occupied) {
						qWarning() << "InventoryService: cell conflict at" << cellCol << "," << cellRow
							<< "for item" << item->id;
						occupiedConflict = true;
						break;
					}

					auto& cell = cells[cellCol][cellRow];
					cell->occupied = true;
					cell->item = item;
				}
			}

			if (!occupiedConflict) {
				// Добавляем предмет в хеш по позиции [0,0]
				items[{item->x, item->y}] = item;
			}
		}
	}


	InventoryItemHandler* itemAt(int col, int row) const {
		if (col < 0 || row < 0 || col >= cols || row >= rows) {
			return nullptr;
		}

		return cells[col][row]->item;
	}

	InventoryItemHandler* itemById(const QString& id) const {
		const auto it = inventoryItems.find(id);
		if (it != inventoryItems.end()) {
			return it->second.get();
		}

		return nullptr;
	}

	InventoryService* q;
	ItemsService* itemsService;
	QString inventoryId;
	QString inventoryName;
	int rows;
	int cols;
	std::map<QString, std::unique_ptr<InventoryItemHandler>> inventoryItems;
	std::vector<std::vector<std::unique_ptr<InventoryViewCell>>> cells;
	QHash<QPoint, InventoryItemHandler*> items;
};

InventoryService::InventoryService(ItemsService* itemsService)
: d(std::make_unique<Private>(this)) {
	d->itemsService = itemsService;
}

InventoryService::~InventoryService() = default;

bool InventoryService::load(const Inventory& inventory) {
	d->rows = inventory.rows;
	d->cols = inventory.cols;
	d->inventoryId = inventory.id;
	d->inventoryName = inventory.name;

	for (const auto &it : inventory.items) {
		const auto item = d->itemsService->itemById(it.id, ItemOwner::Inventory);
		if (!item) {
			qDebug() << "InventoryService::load" << d->inventoryName << d->inventoryId << "can't load item" << item->id;
			continue;
		}

		auto invItem = std::make_unique<InventoryItemHandler>();
		invItem->id = item->id;
		invItem->entity = item->entity;
		invItem->x = it.x;
		invItem->y = it.y;
		invItem->count = it.count;

		d->inventoryItems.emplace(invItem->id, std::move(invItem));
	}

	d->setupCells();
	return true;
}

QString InventoryService::inventoryId() const {
	return d->inventoryId;
}

QString InventoryService::inventoryName() const {
	return d->inventoryName;
}

InventoryService::EntityView InventoryService::items() const {
	return make_deref_view(d->inventoryItems);
}

int InventoryService::rows() const {
	return d->rows;
}

int InventoryService::cols() const {
	return d->cols;
}

const InventoryItemHandler* InventoryService::itemAt(int col, int row) const {
	return d->itemAt(col, row);
}

const InventoryItemHandler* InventoryService::itemById(const QString& id) const {
	return d->itemById(id);
}

bool InventoryService::placeItem(const InventoryItemMimeData& item) {
	auto itemPtr = d->itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	int col = itemPtr->x;
	int row = itemPtr->y;

	const auto availableSpace = canPlaceItem(item, col, row, true);
	if (availableSpace == 0) {
		return false;
	}

	int moveCount = qMin(itemPtr->count, availableSpace);
	auto& targetCell = d->cells[item.x][item.y];
	if (targetCell->occupied) {
		bool canMerge = (
			targetCell->item &&
			targetCell->item->entity->type == itemPtr->entity->type &&
			targetCell->item->id != item.id &&
			itemPtr->entity->maxStack > 1);

		if (canMerge) {
			int spaceInTarget = targetCell->item->entity->maxStack - targetCell->item->count;
			int actualMergeCount = qMin(moveCount, spaceInTarget);
			if (actualMergeCount == 0) {
				return false;
			}

			targetCell->item->count += actualMergeCount;

			emit itemCountChanged(*targetCell->item);
			return true;
		}

		return false;
	}

	for (int dy = 0; dy < itemPtr->entity->height; dy++) {
		for (int dx = 0; dx < itemPtr->entity->width; dx++) {
			// занимаем все ячейки обласи предмета
			// в каждой ячейке занимаемой предметом ссылка на этот предмет,
			auto& cell = d->cells[col + dx][row + dy];
			cell->occupied = true;
			cell->item = itemPtr;
		}
	}

	d->items[{col, row}] = itemPtr;

	itemPtr->x = col;
	itemPtr->y = row;

	emit placeItemEvent(item, row, col);

	return true;
}

int InventoryService::canPlaceItem(const InventoryItemMimeData& item, int col, int row, bool checkItemPlace) const {
	// Проверка границ инвентаря целиком
	if (col < 0 || row < 0 || col + item.width > d->cols || row + item.height > d->rows) {
		return 0;
	}

	// Проверяем все ячейки области предмета
	for (int dy = 0; dy < item.height; dy++) {
		for (int dx = 0; dx < item.width; dx++) {
			int checkX = col + dx;
			int checkY = row + dy;

			const auto& cell = d->cells[checkX][checkY];

			// Если ячейка занята — проверяем условия
			if (cell->occupied) {
				// Случай 1: это та же самая ячейка исходного предмета (при перемещении внутри себя)
				if (checkItemPlace && cell->item && cell->item->id == item.id &&
					checkX >= item.x && checkX < item.x + item.width &&
					checkY >= item.y && checkY < item.y + item.height) {
					continue; // Пропускаем ячейки самого предмета
				}

				// Случай 2: совместимый стекируемый предмет
				if (cell->item && cell->item->entity->type == static_cast<ItemType>(item.type) && item.maxStack > 1) {
					// Возвращаем свободное место в стеке
					return qMax(cell->item->entity->maxStack - cell->item->count, 0);
				}

				// Случай 3: другая занятая ячейка — нельзя разместить ничего
				return 0;
			}
		}
	}

	// Все ячейки свободны — можно разместить полный стек
	return item.count;
}

std::optional<QPoint> InventoryService::findFreeSpace(const InventoryItemMimeData& item, bool checkItemPlace) const {
	for (int row = 0; row < d->rows; row++) {
		for (int col = 0; col < d->cols; col++) {
			const auto count = canPlaceItem(item, col, row, checkItemPlace);
			if (count == item.count) {
				return QPoint(col, row);
			}
		}
	}

	return std::nullopt;
}

void InventoryService::clear() {
	auto keys = d->items.keys();
	for (const auto& pos : keys) {
		if (auto item = d->items.value(pos)) {
			removeItem(*item);
		}
	}

	// Сбрасываем состояние ячеек
	for (int col = 0; col < d->cols; col++) {
		for (int row = 0; row < d->rows; row++) {
			auto &cell = d->cells[col][row];
			cell->occupied = false;
			cell->item = nullptr;
		}
	}

	d->items.clear();
}

void InventoryService::removeItem(const InventoryItemMimeData& item) {
	auto itemIt = d->inventoryItems.find(item.id);
	if (itemIt == d->inventoryItems.end()) {
		return;
	}

	auto itemPtr = itemIt->second.get();
	if (!itemPtr) {
		return;
	}

	// Освобождаем ВСЕ ячейки предмета
	for (int dy = 0; dy < itemPtr->entity->height; ++dy) {
		for (int dx = 0; dx < itemPtr->entity->width; ++dx) {
			int cellCol = itemPtr->x + dx;
			int cellRow = itemPtr->y + dy;
			if (cellCol < d->cols && cellRow < d->rows) {
				auto& cell = d->cells[cellCol][cellRow];
				cell->occupied = false;
				cell->item = nullptr;
			}
		}
	}

	d->items.remove(QPoint(itemPtr->x, itemPtr->y));
	d->inventoryItems.erase(itemIt);

	emit removeItemEvent(item, itemPtr->y, itemPtr->x);
}

bool InventoryService::moveItem(const InventoryItemMimeData& item, int newCol, int newRow, bool checkItemPlace) {
	// Получаем полный предмет из инвентаря по ID
	auto itemPtr = d->itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	// Текущая позиция предмета
	QPoint oldPos(itemPtr->x, itemPtr->y);

	// Проверяем, существует ли предмет в указанной позиции
	if (!d->items.contains(oldPos) || d->items.value(oldPos) != itemPtr) {
		return false;
	}

	// Проверяем возможность размещения (возвращает доступное количество)
	int availableSpace = canPlaceItem(item, newCol, newRow, checkItemPlace);
	if (availableSpace == 0) {
		return false;
	}

	// Определяем сколько реально перемещаем (не больше чем доступно и не больше чем запрошено)
	int moveCount = qMin(item.count, availableSpace);
	int remainingCount = itemPtr->count - moveCount;

	// === Проверяем возможность объединения с существующим стеком в целевой ячейке ===
	auto& targetCell = d->cells[newCol][newRow];
	bool canMerge = (targetCell->occupied &&
		targetCell->item &&
		targetCell->item->entity->type == itemPtr->entity->type &&
		itemPtr->entity->maxStack > 1);

	// === СЛУЧАЙ 1: Объединение стеков (полное или частичное) ===
	if (canMerge) {
		int spaceInTarget = targetCell->item->entity->maxStack - targetCell->item->count;
		int actualMergeCount = qMin(moveCount, spaceInTarget);

		if (actualMergeCount > 0) {
			// Обновляем целевой стек
			targetCell->item->count += actualMergeCount;

			emit itemCountChanged(*targetCell->item);

			// Обновляем исходный стек
			itemPtr->count -= actualMergeCount;

			// Если исходный стек опустел — полностью удаляем его
			if (itemPtr->count == 0) {
				// Освобождаем ВСЕ ячейки исходного предмета
				for (int dy = 0; dy < itemPtr->entity->height; ++dy) {
					for (int dx = 0; dx < itemPtr->entity->width; ++dx) {
						int x = oldPos.x() + dx;
						int y = oldPos.y() + dy;
						if (x < d->cols && y < d->rows) {
							auto& cell = d->cells[x][y];
							cell->occupied = false;
							cell->item = nullptr;
						}
					}
				}

				// Отправляем сигналы
				emit removeItemEvent(*itemPtr, oldPos.x(), oldPos.y());

				// Удаляем из хеша позиций
				d->items.remove(oldPos);

				// Удаляем из основного хранилища (освобождает память через shared_ptr)
				d->inventoryItems.erase(itemPtr->id);

				// Целевой предмет обновлён — можно отправить отдельный сигнал или обновить через представление
				return true;
			}

			emit itemCountChanged(*itemPtr);
			// Если остался остаток — обновляем только количество исходного предмета
			// (позиция не меняется, поэтому не отправляем moveItemEvent)
			return true;
		}
	}

	// === СЛУЧАЙ 2: Полное перемещение без объединения (остаток = 0) ===
	if (remainingCount == 0) {
		// Освобождаем старые ячейки
		for (int dy = 0; dy < itemPtr->entity->height; ++dy) {
			for (int dx = 0; dx < itemPtr->entity->width; ++dx) {
				int x = oldPos.x() + dx;
				int y = oldPos.y() + dy;
				if (x < d->cols && y < d->rows) {
					auto& cell = d->cells[x][y];
					cell->occupied = false;
					cell->item = nullptr;
				}
			}
		}

		// Занимаем новые ячейки
		for (int dy = 0; dy < itemPtr->entity->height; ++dy) {
			for (int dx = 0; dx < itemPtr->entity->width; ++dx) {
				int x = newCol + dx;
				int y = newRow + dy;
				if (x < d->cols && y < d->rows) {
					auto& cell = d->cells[x][y];
					cell->occupied = true;
					cell->item = itemPtr;
				}
			}
		}

		emit moveItemEvent(item, oldPos.x(), oldPos.y(), newCol, newRow);

		// Обновляем позицию в хеш-таблице
		d->items.remove(oldPos);
		d->items[{newCol, newRow}] = itemPtr;

		// Обновляем координаты предмета
		itemPtr->x = newCol;
		itemPtr->y = newRow;
		return true;
	}

	// === СЛУЧАЙ 3: Частичное перемещение без объединения (создаём новый предмет) ===

	// Создаём новый предмет для перемещённой части
	auto newItemPtr = d->duplicateItem(itemPtr->id);
	newItemPtr->count = moveCount;
	newItemPtr->x = newCol;
	newItemPtr->y = newRow;

	// Занимаем целевые ячейки новым предметом
	for (int dy = 0; dy < newItemPtr->entity->height; ++dy) {
		for (int dx = 0; dx < newItemPtr->entity->width; ++dx) {
			int x = newCol + dx;
			int y = newRow + dy;
			if (x < d->cols && y < d->rows) {
				auto& cell = d->cells[x][y];
				cell->occupied = true;
				cell->item = newItemPtr;
			}
		}
	}

	// Добавляем новый предмет в хеш позиций
	d->items[{newCol, newRow}] = newItemPtr;

	// Обновляем исходный предмет (уменьшаем количество)
	itemPtr->count = remainingCount;

	emit itemCountChanged(*itemPtr);

	// Отправляем сигналы
	emit placeItemEvent(*newItemPtr, newRow, newCol); // Новый предмет создан
	// Исходный предмет обновлён (количество уменьшено) — позиция не изменилась
	return true;
}

bool InventoryService::containsItem(const InventoryItemMimeData& item) const {
	auto itemPtr = d->itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	QPoint pos(item.x, item.y);
	if (!d->items.contains(pos)) {
		return false;
	}

	const auto cellItem = d->items.value(pos);
	return cellItem->compare(*itemPtr);
}


bool InventoryService::changeItemsCount(const InventoryItemMimeData& item) {
	auto itemPtr = d->itemById(item.id);
	if (!itemPtr) {		
		return false;
	}

	const auto remains = qMax(itemPtr->count - item.count, 0);
	if (remains == 0) {
		removeItem(item);
	}
	else {
		itemPtr->count = remains;
		emit itemCountChanged(InventoryItemMimeData(*itemPtr));
	}
	
	return true;
}

bool InventoryService::applyItem(const InventoryItemMimeData& item) {
	if (item.count == 0) {
		return false;
	}

	auto invItem = d->duplicateItem(item.id);
	if (!invItem) {
		return false;
	}

	invItem->x = item.x;
	invItem->y = item.y;
	invItem->count = item.count;

	if (!placeItem(InventoryItemMimeData(*invItem))) {
		return false;
	}

	return true;
}
