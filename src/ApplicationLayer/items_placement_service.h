#pragma once
#include <QString>
#include <QPoint>
#include <optional>

class ItemMimeData;

class ItemPlacementAbstraction {
public:
	virtual QString placementId() const = 0;

	virtual bool placeItem(const ItemMimeData& item) = 0;
	virtual int canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const = 0;
	virtual std::optional<QPoint> findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const = 0;

	virtual bool containsItem(const ItemMimeData& item) const = 0;

	virtual bool moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) = 0;
	virtual void removeItem(const ItemMimeData& item) = 0;

	virtual int rows() const = 0;
	virtual int cols() const = 0;


	virtual void clear() = 0;

	virtual bool applyItem(const ItemMimeData& item) = 0;
	virtual bool removeItemsFromStack(const ItemMimeData& item) = 0;
};
