#include "Game/windows_builder.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/controllers.h"
#include "Game/widgets/map_view/map_window.h"
#include "Game/widgets/equipment/equipment_window.h"
#include "Game/widgets/inventory/inventory_window.h"
#include "Game/widgets/items/entities_window.h"

class WindowsBuilder::Private {
public:
	Private(WindowsBuilder* parent)
		: q(parent) {
	}

	WindowsBuilder* q;
	ApplicationController* appController;
};

WindowsBuilder::WindowsBuilder(ApplicationController* appController)
	: d(std::make_unique<Private>(this)) {
	d->appController = appController;
}

WindowsBuilder::~WindowsBuilder() = default;

MdiChildWindow* WindowsBuilder::build(const QString& name, const QString& id) {

	if (name == "map") {
		return new MapWindow(
			d->appController->services()->worldService(),
			d->appController->services()->timeService(),
			id);
	}
	else if (name == "equipment") {
		return new EquipmentWindow(
			d->appController->services()->inventoriesService(),
			id);
	}
	else if (name == "inventory") {
		return new InventoryWindow(
			d->appController->services()->inventoriesService(),
			id);
	}
	else if (name == "items") {
		return new EntitiesWindow(
			d->appController->services()->itemsService(),
			id);
	}

	return nullptr;
}
