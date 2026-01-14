#include "ResourcesTool/main_frame/resources_viewer/tabs_controller.h"
#include <QGuiApplication>

TabsController::TabsController(QTabWidget* tabs, std::shared_ptr<Resources> resources)
: _tabs(tabs)
, _resources(resources)
{
	_tabs->setTabsClosable(true);

	connect(_tabs, &QTabWidget::tabCloseRequested, [&](int index) {
		closeByIndex(index);
		});

	connect(_tabs, &QTabWidget::tabBarClicked, [&](int index) {
			Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
			if (buttons & Qt::MiddleButton) {
				closeByIndex(index);
			}
		});

	connect(_tabs, &QTabWidget::currentChanged, this, &TabsController::onTabChange);
}

void TabsController::closeByIndex(int index)
{
	const auto widget = _tabs->widget(index);
	const auto type = widget->property("tab.type");
	if (type == "home_page") {
		return;
	}

	_tabs->removeTab(index);
}

void TabsController::add(const QString& type, const QString& id)
{
	if (contains(id)) {
		return;
	}


}

bool TabsController::contains(const QString& id)
{
	for (int i = 0; i < _tabs->count(); i++) {
		const auto widget = _tabs->widget(i);
		if (widget && widget->property("tab.id") == id) {
			_tabs->setCurrentIndex(i);
			return true;
		}
	}
	return false;
}

void TabsController::onTabChange(int index)
{
	const auto widget = _tabs->widget(index);
	if (widget == nullptr) {
		return;
	}

	const auto type = widget->property("tab.type").toString();
	const auto id = widget->property("tab.id").toString();
	_current = index;

	emit selectWidget(index, type, id);
}
