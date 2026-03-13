#pragma once
#include <QString>
#include <QString>
#include <QPoint>
#include <optional>

class ItemMimeData;

class ItemPlacementService {
public:

	/// <summary>
	/// Container id
	/// </summary>
	/// <returns></returns>
	virtual QString placementId() const = 0;

	/// <summary>
	/// Check cell to place there current item
	/// </summary>
	/// <param name="item"></param>
	/// <param name="col"></param>
	/// <param name="row"></param>
	/// <param name="checkItemPlace"></param>
	/// <returns></returns>
	virtual int canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const = 0;

	/// <summary>
	/// Find free nearest space in inventory
	/// </summary>
	/// <param name="item"></param>
	/// <param name="checkItemPlace"></param>
	/// <returns></returns>
	virtual std::optional<QPoint> findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const = 0;

	/// <summary>
	/// Check if the cell is empty or not 
	/// </summary>
	/// <param name="item"></param>
	/// <returns></returns>
	virtual bool containsItem(const ItemMimeData& item) const = 0;

	/// <summary>
	/// Move item in new position (newCol, newRow).
	/// If in destination cell has item and allow by type add new items in different cell stack and merge stacks, or move in empty cell.
	/// </summary>
	/// <param name="item"></param>
	/// <param name="newCol"></param>
	/// <param name="newRow"></param>
	/// <param name="checkItemPlace"></param>
	/// <returns></returns>
	virtual bool moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) = 0;

	/// <summary>
	/// Remove item from current container
	/// </summary>
	/// <param name="item"></param>
	virtual void removeItem(const ItemMimeData& item) = 0;

	/// <summary>
	/// Get rows in container
	/// </summary>
	/// <returns></returns>
	virtual int rows() const = 0;

	/// <summary>
	/// Get columns in container
	/// </summary>
	/// <returns></returns>
	virtual int cols() const = 0;

	/// <summary>
	/// Clear all items
	/// </summary>
	virtual void clear() = 0;

	/// <summary>
	/// Place item in container, in item.x and item.y space
	/// </summary>
	/// <param name="item"></param>
	/// <returns></returns>
	virtual bool placeItem(const ItemMimeData& item) = 0;

	/// <summary>
	/// Remove item.
	/// If it is a stack, remove it when the remaining amount is zero, otherwise substract the specified amount
	/// </summary>
	/// <param name="item"></param>
	/// <returns></returns>
	virtual bool removeItemsFromStack(const ItemMimeData& item) = 0;

	/// <summary>
	/// Return item data by id
	/// </summary>
	/// <param name="id"></param>
	/// <returns></returns>
	virtual ItemMimeData itemDataById(const QString& id) const = 0;
};
