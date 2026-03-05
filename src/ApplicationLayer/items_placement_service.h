#pragma once
#include <QString>

class ItemMimeData;

class ItemPlacementAbstraction {
public:
	virtual QString placementId() const = 0;

	virtual bool placeItem(const ItemMimeData& item) = 0;
	int canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const = 0;
};
