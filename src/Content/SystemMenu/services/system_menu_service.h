#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

#include <QObject>
#include <memory>

/// Сервис системного меню.
///
/// Принимает поставщика данных и выполняет операции над пунктами меню:
/// создание, переименование, перемещение, удаление и выборку. Поставщик
/// отвечает только за хранение и ленивую загрузку узлов.
class SystemMenuService : public QObject {
	Q_OBJECT
public:
	explicit SystemMenuService(ILazyNodesDataProvider* provider, QObject* parent = nullptr);
	~SystemMenuService() override;

	ILazyNodesDataProvider* dataProvider() const;

	LazyTreeNodeList rootNodes() const;
	LazyTreeNodeList children(const QVariant& parentId) const;
	LazyTreeNodeList search(const QString& text) const;
	LazyTreeNodePtr item(const QVariant& id) const;

	/// Создать пункт меню; nullptr — операция не выполнена (см. lastError()).
	LazyTreeNodePtr createItem(const QVariant& parentId, const QString& name,
		const QVariantMap& properties = {});

	bool updateItem(const LazyTreeNodePtr& node);
	bool renameItem(const QVariant& id, const QString& name);
	bool moveItem(const QVariant& id, const QVariant& parentId);
	bool removeItem(const QVariant& id);

	QString lastError() const;

signals:
	void errorOccurred(const QString& message);
	void itemChanged(const QVariant& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
