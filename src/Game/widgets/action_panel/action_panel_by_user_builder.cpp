#include "Game/widgets/action_panel/action_panel_by_user_builder.h"
#include "Game/controllers/action_panel_controller.h"

ActionPanelByUserBuilder::ActionPanelByUserBuilder(ActionPanelController* panel)
	: _panel(panel) {
}

void ActionPanelByUserBuilder::build() {
	//d->controller->executeCommandByName("window-create", QStringList{ "map" });
	//d->controller->executeCommandByName("window-create", QStringList{ "equipment", "abfa5aac-7fb5-4570-965f-00af8aee664a" });
	//d->controller->executeCommandByName("window-create", QStringList{ "inventory", "3f2df95f-581b-4084-94bb-20322325e728" });
	//d->controller->executeCommandByName("window-create", QStringList{ "inventory", "b85cf432-ec9d-441e-b438-eab9c5630e4b" });
	//d->controller->executeCommandByName("window-create", QStringList{ "item-entities", "1d6abf2e-9d77-4cf7-9444-2b54aca14259" });

	_panel->clearAllButtons();
	_panel->addButton(ActionButtonConfig("Map", "map.png", "Карта мира", "window-create map", 1));
	_panel->addButton(ActionButtonConfig("Inventory", "inventory.png", "Инвентарь", "window-create inventory 3f2df95f-581b-4084-94bb-20322325e728", 2));
	_panel->addButton(ActionButtonConfig("Chest", "chest.png", "Сундук", "window-create inventory b85cf432-ec9d-441e-b438-eab9c5630e4b", 2));
	_panel->addButton(ActionButtonConfig("Entities", "entities.png", "Библиотека предметов", "window-create item-entities 1d6abf2e-9d77-4cf7-9444-2b54aca14259", 2));
	_panel->addButton(ActionButtonConfig("Equipment", "equipment.png", "Экирировка", "window-create equipment abfa5aac-7fb5-4570-965f-00af8aee664a", 2));
}
