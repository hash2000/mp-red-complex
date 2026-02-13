#include "Game/services/inventory/inventory_service.h"
#include "Game/services/inventory/inventories_service.h"
#include <vector>


struct InventoryViewCell {
	bool occupied = false;
	int row = 0;
	int col = 0;
	InventoryItem* item = nullptr;
};



class InventoryService::Private {
public:
	Private(InventoryService* parent)
	: q(parent) {
	}

	bool checkSpaceOccupied(const InventoryHandler& item, const InventoryViewCell& cell, int checkX, int checkY, bool checkItemPlace) {
		if (cell.occupied) {
			if (checkItemPlace) {
				bool isOldCell = (item.id == cell.item->id) && (checkX >= item.x && checkX < item.x + item.width &&
					checkY >= item.y && checkY < item.y + item.height);
				return !isOldCell;
			}
			return true;
		}
		return false;
	}


	InventoryService* q;
	std::shared_ptr<Inventory> inventory;
	std::vector<std::vector<std::unique_ptr<InventoryViewCell>>> cells;
	QHash<QPoint, InventoryItem*> items;
};

InventoryService::InventoryService(std::shared_ptr<Inventory> inventory)
: d(std::make_unique<Private>(this)) {
	d->inventory = inventory;
	setupCells();
}

InventoryService::~InventoryService() = default;

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
			cell->row = row;
			cell->col = col;
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
				cell->col = item->x;
				cell->row = item->y;
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

	if (!canPlaceItem(item, col, row, true)) {
		return false;
	}

	for (int dy = 0; dy < itemPtr->height; dy++) {
		for (int dx = 0; dx < itemPtr->width; dx++) {
			// занимаем все ячейки обласи предмета
			// в каждой ячейке занимаемой предметом ссылка на этот предмет,
			// но row и col показывают на первую занятую ячейку
			auto& cell = d->cells[col + dx][row + dy];
			cell->occupied = true;
			cell->row = row;
			cell->col = col;
			cell->item = itemPtr.get();			
		}
	}

	d->items[{col, row}] = itemPtr.get();

	itemPtr->x = col;
	itemPtr->y = row;

	emit placeItemEvent(item, row, col);

	return true;
}

bool InventoryService::canPlaceItem(const InventoryHandler& item, int col, int row, bool checkItemPlace) const {
	// Проверка границ инвентаря целиком
	if (col < 0 || row < 0 || col + item.width > d->inventory->cols || row + item.height > d->inventory->rows) {
		return false;
	}

	// проверяем, возможно ли занять предмметом ячейки, которые ему требуются
	for (int dy = 0; dy < item.height; dy++) {
		for (int dx = 0; dx < item.width; dx++) {
			int checkX = col + dx;
			int checkY = row + dy;

			const auto& cell = d->cells[col + dx][row + dy];
			return !d->checkSpaceOccupied(item, *cell, checkX, checkY, checkItemPlace);
		}
	}

	return true;
}

std::optional<QPoint> InventoryService::findFreeSpace(const InventoryHandler& item, bool checkItemPlace) const {
	for (int row = 0; row < d->inventory->rows; row++) {
		for (int col = 0; col < d->inventory->cols; col++) {
			if (canPlaceItem(item, col, row, checkItemPlace)) {
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

	for (int dy = 0; dy < itemPtr->height; dy++) {
		for (int dx = 0; dx < itemPtr->width; dx++) {
			int cellCol = itemPtr->x + dx;
			int cellRow = itemPtr->y + dy;

			if (cellCol < d->inventory->cols && cellRow < d->inventory->rows) {
				auto& cell = d->cells[cellCol][cellRow];
				cell->occupied = false;
				cell->item = nullptr;
				cell->col = 0;
				cell->row = 0;
			}
		}
	}

	d->items.remove(QPoint(itemPtr->x, itemPtr->y));
	d->inventory->items.erase(item.id);

	emit removeItemEvent(item, itemPtr->x, itemPtr->y);
}

bool InventoryService::moveItem(const QString& id, int newCol, int newRow, bool checkItemPlace) {
	auto item = itemById(id);
	if (!item) {
		return false;
	}

	QPoint oldPos(item->x, item->y);
	if (!d->items.contains(oldPos) || !d->items.value(oldPos)->compare(*item)) {
		return false;
	}

	// Проверка границ
	if (newCol < 0 || newRow < 0 ||
		newCol + item->width > d->inventory->cols ||
		newRow + item->height > d->inventory->rows) {
		return false;
	}

	// Проверка занятости (игнорируем старую позицию предмета)
	for (int dy = 0; dy < item->height; dy++) {
		for (int dx = 0; dx < item->width; dx++) {
			int checkX = newCol + dx;
			int checkY = newRow + dy;

			const auto& cell = d->cells[checkX][checkY];
			if (d->checkSpaceOccupied(*item, *cell, checkX, checkY, checkItemPlace)) {
				return false;
			}
		}
	}

	// Освобождаем старые ячейки
	for (int dy = 0; dy < item->height; dy++) {
		for (int dx = 0; dx < item->width; dx++) {
			int x = oldPos.x() + dx;
			int y = oldPos.y() + dy;
			if (x < d->inventory->cols && y < d->inventory->rows) {
				auto& cell = d->cells[x][y];
				cell->occupied = false;
				cell->item = nullptr;
				cell->col = 0;
				cell->row = 0;
			}
		}
	}

	// Занимаем новые ячейки
	for (int dy = 0; dy < item->height; dy++) {
		for (int dx = 0; dx < item->width; dx++) {
			int x = newCol + dx;
			int y = newRow + dy;
			if (x < d->inventory->cols && y < d->inventory->rows) {
				auto& cell = d->cells[x][y];
				cell->occupied = true;
				cell->item = item.get();
				cell->col = newCol;
				cell->row = newRow;
			}
		}
	}

	// Обновляем координаты предмета
	item->x = newCol;
	item->y = newRow;

	d->items.remove(oldPos);
	d->items[{newCol, newRow}] = item.get();

	emit moveItemEvent(*item, oldPos.x(), oldPos.y(), newCol, newRow);
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

