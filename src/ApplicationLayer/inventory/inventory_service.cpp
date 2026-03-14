#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include "ApplicationLayer/items/items_service.h"
#include "DataLayer/items/item.h"
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

	InventoryItemHandler* itemById(const QUuid& id) const {
		const auto it = inventoryItems.find(id);
		if (it != inventoryItems.end()) {
			return it->second.get();
		}

		return nullptr;
	}

	InventoryService* q;
	ItemsService* itemsService;
	QUuid placementId;
	QString inventoryName;
	int rows;
	int cols;
	std::map<QUuid, std::unique_ptr<InventoryItemHandler>> inventoryItems;
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
	d->placementId = inventory.id;
	d->inventoryName = inventory.name;

	for (const auto &it : inventory.items) {
		const auto item = d->itemsService->itemById(it.id);
		if (!item) {
			qDebug() << "InventoryService::load" << d->inventoryName << d->placementId << "can't load item" << it.id;
			continue;
		}

		auto invItem = d->makeInventoryitem(item);
		invItem->x = it.x;
		invItem->y = it.y;
		invItem->count = it.count;

		d->inventoryItems.emplace(invItem->id, std::move(invItem));
	}

	d->setupCells();
	return true;
}

QUuid InventoryService::placementId() const {
	return d->placementId;
}

QString InventoryService::placementName() const {
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

const InventoryItemHandler* InventoryService::itemById(const QUuid& id) const {
	return d->itemById(id);
}

ItemMimeData InventoryService::itemDataById(const QUuid& id) const {
	const auto item = itemById(id);
	if (!item) {
		return ItemMimeData();
	}
	return ItemMimeData(*item);
}

bool InventoryService::placeItem(const ItemMimeData& item) {
	int col = item.x;
	int row = item.y;

	const auto availableSpace = canPlaceItem(item, col, row, true);
	if (availableSpace < item.count) {
		return false;
	}

	const auto itemPtr = d->itemsService->itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	auto invItemPtr = d->itemById(item.id);
	if (!invItemPtr) {
		invItemPtr = d->makeInventoryitem(itemPtr);
		if (!invItemPtr) {
			return false;
		}

		invItemPtr->count = item.count;
		invItemPtr->x = item.x;
		invItemPtr->y = item.y;
	}

	int moveCount = qMin(invItemPtr->count, availableSpace);
	auto& targetCell = d->cells[col][row];
	if (targetCell->occupied) {
		bool canMerge = (
			targetCell->item &&
			targetCell->item->entity->type == invItemPtr->entity->type &&
			targetCell->item->entity->id == invItemPtr->entity->id &&
			targetCell->item->id != item.id &&
			invItemPtr->entity->maxStack > 1);

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

	for (int dy = 0; dy < invItemPtr->entity->height; dy++) {
		for (int dx = 0; dx < invItemPtr->entity->width; dx++) {
			// занимаем все ячейки обласи предмета
			// в каждой ячейке занимаемой предметом ссылка на этот предмет,
			auto& cell = d->cells[col + dx][row + dy];
			cell->occupied = true;
			cell->item = invItemPtr;
		}
	}

	d->items[{col, row}] = invItemPtr;

	invItemPtr->x = col;
	invItemPtr->y = row;

	emit placeItemEvent(item, col, row);

	return true;
}

int InventoryService::canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const {
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
					checkX >= row && checkX < row + item.width &&
					checkY >= col && checkY < col + item.height) {
					continue; // Пропускаем ячейки самого предмета
				}

				// Случай 2: совместимый стекируемый предмет
				if (cell->item &&
					cell->item->entity->type == static_cast<ItemType>(item.type) &&
					cell->item->entity->id == item.entityId &&
					item.maxStack > 1) {
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

std::optional<QPoint> InventoryService::findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const {
	for (int row = 0; row < d->rows; row++) {
		for (int col = 0; col < d->cols; col++) {
			const auto count = canPlaceItem(item, col, row, checkItemPlace);
			if (count >= item.count) {
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

void InventoryService::removeItem(const ItemMimeData& item) {
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

bool InventoryService::moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) {
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
		targetCell->item->entity->id == itemPtr->entity->id &&
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
				emit removeItemEvent(*itemPtr, oldPos.y(), oldPos.x());

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

		emit moveItemEvent(item, oldPos.y(), oldPos.x(), newCol, newRow);

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
	const auto newItem = d->itemsService->duplicate(itemPtr->id);
	if (!newItem) {
		return false;
	}

	auto newItemPtr = d->makeInventoryitem(newItem);
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
	emit placeItemEvent(*newItemPtr, newCol, newRow); // Новый предмет создан
	// Исходный предмет обновлён (количество уменьшено) — позиция не изменилась
	return true;
}

bool InventoryService::containsItem(const ItemMimeData& item) const {
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


bool InventoryService::removeItemsFromStack(const ItemMimeData& item) {
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
		emit itemCountChanged(ItemMimeData(*itemPtr));
	}
	
	return true;
}
