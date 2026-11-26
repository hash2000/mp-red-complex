#include "Content/SystemRouter/widgets/system_router_tree_widget.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"

#include <QAction>
#include <QMenu>

class SystemRouterTreeWidget::Private {
public:
	explicit Private(SystemRouterTreeWidget* parent)
		: q(parent) {
	}

	SystemRouterTreeWidget* q;
	SystemRouterService* service = nullptr;
};

SystemRouterTreeWidget::SystemRouterTreeWidget(SystemRouterService* service, QWidget* parent)
	: LazyTreeWidget(service ? service->dataProvider() : nullptr, parent)
	, d(std::make_unique<Private>(this)) {
	d->service = service;
	setObjectName(QStringLiteral("SystemRouterTreeWidget"));
	// Двойной щелчок выполняет скрипт узла, а не раскрывает его.
	treeView()->setExpandsOnDoubleClick(false);
}

SystemRouterTreeWidget::~SystemRouterTreeWidget() = default;

SystemRouterService* SystemRouterTreeWidget::service() const {
	return d->service;
}

void SystemRouterTreeWidget::extendContextMenu(QMenu& menu, const LazyTreeNodePtr& node) {
	if (!node) {
		return;
	}

	menu.addSeparator();
	QAction* view = menu.addAction(tr("Просмотр…"));
	view->setToolTip(tr("Показать параметры узла без возможности редактирования"));
	connect(view, &QAction::triggered, this, [this, node] {
		emit viewRequested(node);
	});

	QAction* properties = menu.addAction(tr("Свойства…"));
	connect(properties, &QAction::triggered, this, [this, node] {
		emit propertiesRequested(node);
	});
}
