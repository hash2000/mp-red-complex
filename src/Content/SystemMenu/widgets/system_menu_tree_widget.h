#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"

#include <memory>

class SystemMenuService;

/// Дерево системного меню.
///
/// Расширяет LazyTreeWidget: знает сервис системного меню, но не зависит от
/// документа и его контекста. Документ оборачивает виджет и ведёт состояние.
class SystemMenuTreeWidget final : public LazyTreeWidget {
	Q_OBJECT
public:
	explicit SystemMenuTreeWidget(SystemMenuService* service, QWidget* parent = nullptr);
	~SystemMenuTreeWidget() override;

	SystemMenuService* service() const;

signals:
	/// Открыть параметры узла: двойной щелчок/Enter или пункт меню.
	void propertiesRequested(const LazyTreeNodePtr& node);

protected:
	void extendContextMenu(QMenu& menu, const LazyTreeNodePtr& node) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
