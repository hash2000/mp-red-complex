#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

#include <QObject>
#include <memory>

/// Сервис сохранённых запросов FetchApi.
///
/// Принимает поставщика данных и не наследует его: сервис отвечает за
/// операции предметной области и единое место проверки ошибок, а поставщик —
/// за хранение и ленивую загрузку.
class FetchQueriesService : public QObject {
	Q_OBJECT
public:
	explicit FetchQueriesService(ILazyNodesDataProvider* provider, QObject* parent = nullptr);
	~FetchQueriesService() override;

	ILazyNodesDataProvider* dataProvider() const;

	LazyTreeNodeList queries(const QVariant& parentId = {}) const;
	LazyTreeNodeList search(const QString& text) const;
	LazyTreeNodePtr query(const QVariant& id) const;

	bool addQuery(const LazyTreeNodePtr& node);
	bool renameQuery(const QVariant& id, const QString& name);
	bool removeQuery(const QVariant& id);
	bool moveQuery(const QVariant& id, const QVariant& parentId);

	QString lastError() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
