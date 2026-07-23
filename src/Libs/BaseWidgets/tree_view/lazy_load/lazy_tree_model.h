#pragma once
#include <QAbstractItemModel>
#include <QModelIndex>

class ILazyTreeNode;
class ILazyNodesDataProvider;
class LazyTreeItem;

class LazyTreeModel : public QAbstractItemModel {
	Q_OBJECT
public:
	explicit LazyTreeModel(ILazyNodesDataProvider* provider, QObject* parent = nullptr);
	~LazyTreeModel() override;

	QVariant data(const QModelIndex& index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	bool canFetchMore(const QModelIndex& parent) const override;
	void fetchMore(const QModelIndex& parent) override;

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	QModelIndex appendTemporaryItem(const QModelIndex& currentIndex);

	// Поиск индекса по ID (для возврата из поиска)
	QModelIndex findIndexById(int id, const QModelIndex& parent = QModelIndex()) const;

	LazyTreeItem* getItem(const QModelIndex& index) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
