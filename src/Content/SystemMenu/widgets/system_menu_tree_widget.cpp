#include "Content/SystemMenu/widgets/system_menu_tree_widget.h"
#include "Content/SystemMenu/services/system_menu_service.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"

#include <QAction>
#include <QMenu>

class SystemMenuTreeWidget::Private {
public:
	explicit Private(SystemMenuTreeWidget* parent)
		: q(parent) {
	}

	SystemMenuTreeWidget* q;
	SystemMenuService* service = nullptr;
};

SystemMenuTreeWidget::SystemMenuTreeWidget(SystemMenuService* service, QWidget* parent)
	: LazyTreeWidget(service ? service->dataProvider() : nullptr, parent)
	, d(std::make_unique<Private>(this)) {
	d->service = service;
	setObjectName(QStringLiteral("SystemMenuTreeWidget"));
	// Двойной щелчок открывает параметры узла, а не раскрывает его.
	treeView()->setExpandsOnDoubleClick(false);

	connect(this, &LazyTreeWidget::nodeActivated, this,
		[this](const LazyTreeNodePtr& node) {
			if (node) {
				emit propertiesRequested(node);
			}
		});
}

SystemMenuTreeWidget::~SystemMenuTreeWidget() = default;

SystemMenuService* SystemMenuTreeWidget::service() const {
	return d->service;
}

void SystemMenuTreeWidget::extendContextMenu(QMenu& menu, const LazyTreeNodePtr& node) {
	if (!node) {
		return;
	}

	menu.addSeparator();
	QAction* properties = menu.addAction(tr("Свойства…"));
	connect(properties, &QAction::triggered, this, [this, node] {
		emit propertiesRequested(node);
	});
}
