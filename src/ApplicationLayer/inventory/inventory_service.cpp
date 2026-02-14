#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventories_service.h"
#include <vector>


struct InventoryViewCell {
	bool occupied = false;
	InventoryItem* item = nullptr;
};


class InventoryService::Private {
public:
	Private(InventoryService* parent)
	: q(parent) {
	}

	InventoryService* q;
	InventoriesController* controller;
	std::shared_ptr<Inventory> inventory;
	std::vector<std::vector<std::unique_ptr<InventoryViewCell>>> cells;
	QHash<QPoint, InventoryItem*> items;
};

InventoryService::InventoryService(std::shared_ptr<Inventory> inventory, InventoriesController* controller)
: d(std::make_unique<Private>(this)) {
	d->inventory = inventory;
	d->controller = controller;
	setupCells();
}

InventoryService::~InventoryService() = default;

InventoriesController* InventoryService::controller() const {
	return d->controller;
}

std::shared_ptr<Inventory> InventoryService::inventory() const {
	return d->inventory;
}

void InventoryService::setupCells() {
	if (!d->inventory) {
		d->cells.clear();
		d->items.clear();
		return;
	}

	d->cells.resize(d->inventory->cols);
	for (int col = 0; col < d->inventory->cols; col++) {
		d->cells[col].resize(d->inventory->rows);
		for (int row = 0; row < d->inventory->rows; row++) {
			auto cell = std::make_unique<InventoryViewCell>();
			d->cells[col][row] = std::move(cell);
		}
	}

	d->items.clear();

	for (auto itemIt : d->inventory->items) {
		auto item = itemIt.second;
		if (!item) {
			continue;
		}

		// Проверка валидности координат предмета
		if (item->x < 0 || item->y < 0 ||
			item->x + item->width > d->inventory->cols ||
			item->y + item->height > d->inventory->rows) {
			qWarning() << "InventoryService: item" << item->id
				<< "has invalid position (" << item->x << "," << item->y
				<< ") for grid" << d->inventory->cols << "x" << d->inventory->rows;
			continue;
		}

		// Занимаем ячейки предметом
		bool occupiedConflict = false;
		for (int dy = 0; dy < item->height && !occupiedConflict; dy++) {
			for (int dx = 0; dx < item->width && !occupiedConflict; dx++) {
				int cellCol = item->x + dx;
				int cellRow = item->y + dy;

				if (d->cells[cellCol][cellRow]->occupied) {
					qWarning() << "InventoryService: cell conflict at" << cellCol << "," << cellRow
						<< "for item" << item->id;
					occupiedConflict = true;
					break;
				}

				auto& cell = d->cells[cellCol][cellRow];
				cell->occupied = true;
				cell->item = item.get();
			}
		}

		if (!occupiedConflict) {
			// Добавляем предмет в хеш по позиции [0,0]
			d->items[{item->x, item->y}] = item.get();
		}
	}
}

bool InventoryService::placeItem(const InventoryHandler& item) {
	auto itemPtr = itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	int col = itemPtr->x;
	int row = itemPtr->y;

	const auto availableSpace = canPlaceItem(item, col, row, true);
	if (availableSpace == 0) {
		return false;
	}

	int moveCount = qMin(item.count, availableSpace);
	auto& targetCell = d->cells[item.x][item.y];
	if (targetCell->occupied) {
		bool canMerge = (
			targetCell->item &&
			targetCell->item->type == item.type &&
			targetCell->item->id != item.id &&
			item.maxStack > 1);

		if (canMerge) {
			int spaceInTarget = targetCell->item->maxStack - targetCell->item->count;
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

	for (int dy = 0; dy < itemPtr->height; dy++) {
		for (int dx = 0; dx < itemPtr->width; dx++) {
			// занимаем все ячейки обласи предмета
			// в каждой ячейке занимаемой предметом ссылка на этот предмет,
			auto& cell = d->cells[col + dx][row + dy];
			cell->occupied = true;
			cell->item = itemPtr.get();
		}
	}

	d->items[{col, row}] = itemPtr.get();

	itemPtr->x = col;
	itemPtr->y = row;

	emit placeItemEvent(item, row, col);

	return true;
}

int InventoryService::canPlaceItem(const InventoryHandler& item, int col, int row, bool checkItemPlace) const {
	// Проверка границ инвентаря целиком
	if (col < 0 || row < 0 || col + item.width > d->inventory->cols || row + item.height > d->inventory->rows) {
		return 0;
	}

	// Проверяем все ячейки области предмета
	for (int dy = 0; dy < item.height; ++dy) {
		for (int dx = 0; dx < item.width; ++dx) {
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
				if (cell->item && cell->item->type == item.type && item.maxStack > 1) {
					// Возвращаем свободное место в стеке
					return qMax(cell->item->maxStack - cell->item->count, 0);
				}

				// Случай 3: другая занятая ячейка — нельзя разместить ничего
				return 0;
			}
		}
	}

	// Все ячейки свободны — можно разместить полный стек
	return item.count;
}

std::optional<QPoint> InventoryService::findFreeSpace(const InventoryHandler& item, bool checkItemPlace) const {
	for (int row = 0; row < d->inventory->rows; row++) {
		for (int col = 0; col < d->inventory->cols; col++) {
			const auto count = canPlaceItem(item, col, row, checkItemPlace);
			if (count == item.count) {
				return QPoint(col, row);
			}
		}
	}

	return std::nullopt;
}

InventoryItem* InventoryService::itemAt(int col, int row) const {
	if (col < 0 || row < 0 || col >= d->inventory->cols || row >= d->inventory->rows) {
		return nullptr;
	}

	return d->cells[col][row]->item;
}

void InventoryService::clear() {
	auto keys = d->items.keys();
	for (const auto& pos : keys) {
		if (auto item = d->items.value(pos)) {
			removeItem(*item);
		}
	}

	// Сбрасываем состояние ячеек
	for (int col = 0; col < d->inventory->cols; col++) {
		for (int row = 0; row < d->inventory->rows; row++) {
			auto &cell = d->cells[col][row];
			cell->occupied = false;
			cell->item = nullptr;
		}
	}

	d->items.clear();
}

void InventoryService::removeItem(const InventoryHandler& item) {
	auto itemIt = d->inventory->items.find(item.id);
	if (itemIt == d->inventory->items.end()) {
		return;
	}

	auto itemPtr = itemIt->second;
	if (!itemPtr) {
		return;
	}

	// Освобождаем ВСЕ ячейки предмета
	for (int dy = 0; dy < itemPtr->height; ++dy) {
		for (int dx = 0; dx < itemPtr->width; ++dx) {
			int cellCol = itemPtr->x + dx;
			int cellRow = itemPtr->y + dy;
			if (cellCol < d->inventory->cols && cellRow < d->inventory->rows) {
				auto& cell = d->cells[cellCol][cellRow];
				cell->occupied = false;
				cell->item = nullptr;
			}
		}
	}

	d->items.remove(QPoint(itemPtr->x, itemPtr->y));
	d->inventory->items.erase(itemIt);

	emit removeItemEvent(item, itemPtr->y, itemPtr->x);
}

bool InventoryService::moveItem(const InventoryHandler& item, int newCol, int newRow, bool checkItemPlace) {
	// Получаем полный предмет из инвентаря по ID
	auto itemPtr = itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	// Текущая позиция предмета
	QPoint oldPos(itemPtr->x, itemPtr->y);

	// Проверяем, существует ли предмет в указанной позиции
	if (!d->items.contains(oldPos) || d->items.value(oldPos) != itemPtr.get()) {
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
		targetCell->item->type == item.type &&
		targetCell->item->id != item.id &&
		item.maxStack > 1);

	// === СЛУЧАЙ 1: Объединение стеков (полное или частичное) ===
	if (canMerge) {
		int spaceInTarget = targetCell->item->maxStack - targetCell->item->count;
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
				for (int dy = 0; dy < itemPtr->height; ++dy) {
					for (int dx = 0; dx < itemPtr->width; ++dx) {
						int x = oldPos.x() + dx;
						int y = oldPos.y() + dy;
						if (x < d->inventory->cols && y < d->inventory->rows) {
							auto& cell = d->cells[x][y];
							cell->occupied = false;
							cell->item = nullptr;
						}
					}
				}

				// Удаляем из хеша позиций
				d->items.remove(oldPos);

				// Удаляем из основного хранилища (освобождает память через shared_ptr)
				d->inventory->items.erase(itemPtr->id);

				// Отправляем сигналы
				emit removeItemEvent(*itemPtr, oldPos.x(), oldPos.y());
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
		for (int dy = 0; dy < itemPtr->height; ++dy) {
			for (int dx = 0; dx < itemPtr->width; ++dx) {
				int x = oldPos.x() + dx;
				int y = oldPos.y() + dy;
				if (x < d->inventory->cols && y < d->inventory->rows) {
					auto& cell = d->cells[x][y];
					cell->occupied = false;
					cell->item = nullptr;
				}
			}
		}

		// Занимаем новые ячейки
		for (int dy = 0; dy < itemPtr->height; ++dy) {
			for (int dx = 0; dx < itemPtr->width; ++dx) {
				int x = newCol + dx;
				int y = newRow + dy;
				if (x < d->inventory->cols && y < d->inventory->rows) {
					auto& cell = d->cells[x][y];
					cell->occupied = true;
					cell->item = itemPtr.get();
				}
			}
		}

		// Обновляем позицию в хеш-таблице
		d->items.remove(oldPos);
		d->items[{newCol, newRow}] = itemPtr.get();

		// Обновляем координаты предмета
		itemPtr->x = newCol;
		itemPtr->y = newRow;

		emit moveItemEvent(item, oldPos.x(), oldPos.y(), newCol, newRow);
		return true;
	}

	// === СЛУЧАЙ 3: Частичное перемещение без объединения (создаём новый предмет) ===

	// Создаём новый предмет для перемещённой части
	auto newItemPtr = itemPtr->duplicate(true); // Создаём копию с новым уникальным ID
	newItemPtr->count = moveCount;
	newItemPtr->x = newCol;
	newItemPtr->y = newRow;

	// Добавляем новый предмет в инвентарь
	d->inventory->items.emplace(newItemPtr->id, newItemPtr);

	// Занимаем целевые ячейки новым предметом
	for (int dy = 0; dy < newItemPtr->height; ++dy) {
		for (int dx = 0; dx < newItemPtr->width; ++dx) {
			int x = newCol + dx;
			int y = newRow + dy;
			if (x < d->inventory->cols && y < d->inventory->rows) {
				auto& cell = d->cells[x][y];
				cell->occupied = true;
				cell->item = newItemPtr.get();
			}
		}
	}

	// Добавляем новый предмет в хеш позиций
	d->items[{newCol, newRow}] = newItemPtr.get();

	// Обновляем исходный предмет (уменьшаем количество)
	itemPtr->count = remainingCount;

	// Отправляем сигналы
	emit placeItemEvent(*newItemPtr, newRow, newCol); // Новый предмет создан
	// Исходный предмет обновлён (количество уменьшено) — позиция не изменилась
	return true;
}

QVector<InventoryItem*> InventoryService::items() const {
	QVector<InventoryItem*> result;
	result.reserve(d->items.size());
	for (auto item : d->items) {
		if (item) {
			result.append(item);
		}
	}
	return result;
}

bool InventoryService::containsItem(const InventoryHandler& item) const {
	QPoint pos(item.x, item.y);

	if (!d->items.contains(pos)) {
		return false;
	}

	const auto cellItem = d->items.value(pos);
	return cellItem->compare(item);
}

std::shared_ptr<InventoryItem> InventoryService::itemById(const QString& id) const {
	const auto it = d->inventory->items.find(id);
	if (it != d->inventory->items.end()) {
		return it->second;
	}

	return std::shared_ptr<InventoryItem>();
}

bool InventoryService::splitStack(const QString& itemId, int newCol, int newRow, int splitCount) {
	auto itemPtr = itemById(itemId);
	if (!itemPtr || splitCount <= 0 || splitCount >= itemPtr->count) {
		return false;
	}

	// Проверяем возможность размещения
	InventoryHandler handler;
	handler.id = itemId;
	handler.x = newCol;
	handler.y = newRow;
	handler.width = itemPtr->width;
	handler.height = itemPtr->height;
	handler.count = splitCount;
	handler.maxStack = itemPtr->maxStack;

	if (canPlaceItem(handler, newCol, newRow, false) < splitCount) {
		return false;
	}

	// Создаём новый предмет для разделённой части
	auto newItemPtr = itemPtr->duplicate(true);
	newItemPtr->count = splitCount;
	newItemPtr->x = newCol;
	newItemPtr->y = newRow;

	// Добавляем в инвентарь
	d->inventory->items.emplace(newItemPtr->id, newItemPtr);

	// Занимаем ячейки
	for (int dy = 0; dy < newItemPtr->height; ++dy) {
		for (int dx = 0; dx < newItemPtr->width; ++dx) {
			int x = newCol + dx;
			int y = newRow + dy;
			if (x < d->inventory->cols && y < d->inventory->rows) {
				auto& cell = d->cells[x][y];
				cell->occupied = true;
				cell->item = newItemPtr.get();
			}
		}
	}

	d->items[{newCol, newRow}] = newItemPtr.get();

	// Уменьшаем количество в исходном предмете
	itemPtr->count -= splitCount;

	emit placeItemEvent(*newItemPtr, newRow, newCol);
	return true;
}
