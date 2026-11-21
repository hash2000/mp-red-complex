#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

#include <QString>
#include <memory>

class DatabasesService;

/// Поставщик узлов дерева для типовой таблицы SQLite.
///
/// Ожидаемая структура таблицы:
///   id        INTEGER PRIMARY KEY AUTOINCREMENT
///   parent_id INTEGER NULL REFERENCES <table>(id)
///   name      TEXT NOT NULL
///   data      TEXT NOT NULL — JSON-объект со свойствами узла ({} для пустых)
///   expanded  INTEGER NOT NULL DEFAULT 0 — признак «узел всегда открыт»
///
/// Узлы с expanded = 1 загружаются сразу вместе с вложенными узлами, поэтому
/// при загрузке дерева пользователь видит раскрытыми «важные» для него ветки.
/// Признак меняется методом setExpanded(); update() его не трогает — он же
/// сохраняет служебное раскрытие из результатов поиска и из самого дерева.
///
/// Поставщик не является сервисом: его создаёт фабрика сервисов и передаёт
/// сервису/виджету. Специфичные для предметной области узлы добавляются
/// переопределением createTreeNode(), пост-обработка вставки — onNodeInserted().
class LazyNodesDataProviderDb : public ILazyNodesDataProvider {
public:
	LazyNodesDataProviderDb(DatabasesService* databasesService, const QString& databaseName,
		const QString& tableName);
	~LazyNodesDataProviderDb() override;

	LazyTreeNodePtr createTreeNode() override;
	LazyTreeNodePtr createRootNode() override;

	LazyTreeNodeList get(const QVariant& parentId) const override;
	LazyTreeNodePtr getNode(const QVariant& id) const override;
	LazyTreeNodeList search(const QString& text) const override;
	bool add(const LazyTreeNodePtr& node) override;
	bool remove(const QVariant& id) override;
	bool update(const LazyTreeNodePtr& node) override;

	/// Сохранить признак «узел всегда открыт» в поле expanded.
	bool setExpanded(const QVariant& id, bool expanded) override;

	QString lastError() const override;

protected:
	/// Вызывается после успешной вставки; идентификатор узла уже присвоен.
	virtual void onNodeInserted(const LazyTreeNodePtr& node, qint64 lastInsertId);

	void setLastError(const QString& error) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
