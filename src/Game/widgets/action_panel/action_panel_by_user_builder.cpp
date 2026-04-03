#include "Game/widgets/action_panel/action_panel_by_user_builder.h"
#include "Game/controllers/action_panel_controller.h"
#include "ApplicationLayer/users/users_service.h"

class ActionPanelByUserBuilder::Private {
public:
	Private(ActionPanelByUserBuilder* parent) : q(parent) { }

	void addChestButton() {
		QString cmd = QString("window-create inventory %1")
			.arg(usersService->getChestId().toString(QUuid::StringFormat::WithoutBraces));

		panel->addButton(ActionButtonConfig("Chest", "chest.png", "Сундук", cmd, 2));
	}

	ActionPanelByUserBuilder* q;
	ActionPanelController* panel;
	UsersService* usersService;
};


ActionPanelByUserBuilder::ActionPanelByUserBuilder(ActionPanelController* panel, UsersService* usersService)
: d(std::make_unique<Private>(this)) {
	d->panel = panel;
	d->usersService = usersService;
}

ActionPanelByUserBuilder::~ActionPanelByUserBuilder() = default;

void ActionPanelByUserBuilder::build() {
	d->panel->clearAllButtons();
	if (!d->usersService->isAuthenticated()) {
		return;
	}

	d->addChestButton();

	//d->panel->addButton(ActionButtonConfig("Map", "map.png", "Карта мира", "window-create map", 1));
	//d->panel->addButton(ActionButtonConfig("Inventory", "inventory.png", "Инвентарь", "window-create inventory 3f2df95f-581b-4084-94bb-20322325e728", 2));
	//d->panel->addButton(ActionButtonConfig("Chest", "chest.png", "Сундук", "window-create inventory b85cf432-ec9d-441e-b438-eab9c5630e4b", 2));
	//d->panel->addButton(ActionButtonConfig("Entities", "entities.png", "Библиотека предметов", "window-create item-entities 1d6abf2e-9d77-4cf7-9444-2b54aca14259", 2));
	//d->panel->addButton(ActionButtonConfig("Equipment", "equipment.png", "Экирировка", "window-create equipment abfa5aac-7fb5-4570-965f-00af8aee664a", 2));

}
