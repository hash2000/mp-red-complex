#pragma once
#include <QStandardItemModel>

/// Модель ленивого дерева.
///
/// Отвечает только за данные: MIME-тип перетаскивания и проверку допустимости
/// сброса. Само перемещение выполняет LazyTreeView::dropEvent, чтобы сначала
/// сохранить новый родитель в поставщике данных и не оставлять дерево
/// в состоянии, расходящемся с базой.
class LazyTreeModel : public QStandardItemModel {
	Q_OBJECT
public:
	static const QString MimeType;

	explicit LazyTreeModel(QObject* parent = nullptr);

	/// Поиск элемента по идентификатору узла.
	/// Скрытый корневой элемент и заглушки не участвуют в поиске.
	QStandardItem* findItemById(const QVariant& id) const;

protected:
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	QStringList mimeTypes() const override;
	bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
		const QModelIndex& parent) const override;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
		const QModelIndex& parent) override;
	Qt::DropActions supportedDropActions() const override;

private:
	bool isDescendantOf(QStandardItem* node, QStandardItem* target) const;
};

inline const QString LazyTreeModel::MimeType = "application/x-lazy-tree-node-id";
