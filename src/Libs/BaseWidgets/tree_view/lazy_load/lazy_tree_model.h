#pragma once
#include <QStandardItemModel>

class LazyTreeModel : public QStandardItemModel {
	Q_OBJECT
public:
	static const QString MimeType;
public:
	explicit LazyTreeModel(QObject* parent = nullptr);
	QStandardItem* findItemById(const QVariant& id) const;

protected:
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	Qt::DropActions supportedDropActions() const override;
	bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

private:
	bool isDescendantOf(QStandardItem* node, QStandardItem* target) const;
};

inline const QString LazyTreeModel::MimeType = "application/x-lazy-tree-node-id";
