#include "Game/windows_builder.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/controllers.h"
#include "Game/widgets/map_view/map_window.h"
#include "Game/widgets/equipment/equipment_window.h"
#include "Game/widgets/inventory/inventory_window.h"
#include "Game/widgets/items/entities_window.h"
#include "Game/widgets/user/login_window.h"
#include "Game/widgets/user/user_window.h"
#include <map>
#include <functional>

class WindowsBuilder::Private {
public:
	Private(WindowsBuilder* parent)
		: q(parent) {
	}

	WindowsBuilder* q;
	ApplicationController* appController;
	std::map<QString, std::function<MdiChildWindow*(Services*, const QString&, QWidget*)>> factory;
};

WindowsBuilder::WindowsBuilder(ApplicationController* appController)
: d(std::make_unique<Private>(this)) {
	d->appController = appController;

	d->factory.emplace("map", [](Services* services, const QString& id, QWidget* parent) {
		return new MapWindow(
			services->worldService(),
			services->timeService(),
			id,
			parent);
		});
	d->factory.emplace("equipment", [](Services* services, const QString& id, QWidget* parent) {
		return new EquipmentWindow(
			services->inventoriesService(),
			id,
			parent);
		});
	d->factory.emplace("inventory", [](Services* services, const QString& id, QWidget* parent) {
		return new InventoryWindow(
			services->inventoriesService(),
			id,
			parent);
		});
	d->factory.emplace("item-entities", [](Services* services, const QString& id, QWidget* parent) {
		return new EntitiesWindow(
			services->itemsService(),
			id,
			parent);
		});
	d->factory.emplace("user-login", [](Services* services, const QString& id, QWidget* parent) {
		return new LoginWindow(
			services->usersService(),
			id,
			parent);
		});
	d->factory.emplace("user-profile", [](Services* services, const QString& id, QWidget* parent) {
		return new UserWindow(
			services->usersService(),
			services->texturesService(),
			id,
			parent);
		});
}

WindowsBuilder::~WindowsBuilder() = default;

MdiChildWindow* WindowsBuilder::build(const QString& name, const QString& id, QWidget* parent) {
	const auto &it = d->factory.find(name);
	if (it == d->factory.end()) {
		return nullptr;
	}

	return it->second(d->appController->services(), id, parent);
}
