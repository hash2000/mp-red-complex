#include "Content/SystemMenu/widgets/system_menu_tree_widget.h"
#include "Content/SystemMenu/services/system_menu_service.h"

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
}

SystemMenuTreeWidget::~SystemMenuTreeWidget() = default;

SystemMenuService* SystemMenuTreeWidget::service() const {
	return d->service;
}
