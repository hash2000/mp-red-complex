#include "Game/windows_builder.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/widgets/map_view/map_window.h"
#include "Game/widgets/map_view/map_editor_window.h"
#include "Game/widgets/equipment/equipment_window.h"
#include "Game/widgets/inventory/inventory_window.h"
#include "Game/widgets/items/entities_window.h"
#include "Game/widgets/user/login_window.h"
#include "Game/widgets/user/user_window.h"
#include "Game/widgets/warmup/warmup_window.h"
#include "Game/widgets/textures/texture_editor_window.h"
#include "Game/widgets/materials/materials_window.h"
#include "Game/widgets/code_editor/code_editor_window.h"

#include "ApplicationLayer/shaders/shaders_service.h"

#include <map>
#include <functional>

class WindowsBuilder::Private {
public:
	Private(WindowsBuilder* parent)
		: q(parent) {
	}

	WindowsBuilder* q;
	ApplicationController* appController;
	std::map<QString, std::function<MdiChildWindow*(const QString&, QWidget*)>> factory;
};

WindowsBuilder::WindowsBuilder(ApplicationController* appController)
: d(std::make_unique<Private>(this)) {
	d->appController = appController;

	d->factory.emplace("map", [](const QString& id, QWidget* parent) {
		return new MapWindow(id, parent);
		});
	d->factory.emplace("equipment", [](const QString& id, QWidget* parent) {
		return new EquipmentWindow(id, parent);
		});
	d->factory.emplace("inventory", [](const QString& id, QWidget* parent) {
		return new InventoryWindow(id, parent);
		});
	d->factory.emplace("item-entities", [](const QString& id, QWidget* parent) {
		return new EntitiesWindow(id,	parent);
		});
	d->factory.emplace("user-login", [](const QString& id, QWidget* parent) {
		return new LoginWindow(id, parent);
		});
	d->factory.emplace("user-profile", [](const QString& id, QWidget* parent) {
		return new UserWindow(id,	parent);
		});
	d->factory.emplace("warmup", [](const QString& id, QWidget* parent) {
		return new WarmupWindow(id, parent);
		});
	d->factory.emplace("texture-editor", [](const QString& id, QWidget* parent) {
		return new TextureEditorWindow(id, parent);
		});
	d->factory.emplace("map-editor", [](const QString& id, QWidget* parent) {
		return new MapEditorWindow(id, parent);
		});
	d->factory.emplace("material-editor", [](const QString& id, QWidget* parent) {
		return new MaterialsWindow(id, parent);
		});
	d->factory.emplace("code-editor", [](const QString& id, QWidget* parent) {
		return new CodeEditorWindow(id, parent);
		});
}

WindowsBuilder::~WindowsBuilder() = default;

MdiChildWindow* WindowsBuilder::build(const QString& name, const QString& id, QWidget* parent) {
	const auto &it = d->factory.find(name);
	if (it == d->factory.end()) {
		return nullptr;
	}

	return it->second(id, parent);
}
