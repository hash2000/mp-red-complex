#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"

#include <memory>

class SystemRouterService;

/// Дерево системного маршрутизатора.
///
/// Расширяет LazyTreeWidget: знает сервис системного маршрутизатора, но не
/// зависит от документа и его контекста. Документ оборачивает виджет и ведёт
/// состояние.
class SystemRouterTreeWidget final : public LazyTreeWidget {
	Q_OBJECT
public:
	explicit SystemRouterTreeWidget(SystemRouterService* service, QWidget* parent = nullptr);
	~SystemRouterTreeWidget() override;

	SystemRouterService* service() const;

signals:
	/// Открыть параметры узла (редактирование) из контекстного меню.
	void propertiesRequested(const LazyTreeNodePtr& node);
	/// Открыть просмотр параметров узла из контекстного меню.
	void viewRequested(const LazyTreeNodePtr& node);

protected:
	void extendContextMenu(QMenu& menu, const LazyTreeNodePtr& node) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
