#include "Game/widgets/action_panel/action_panel_by_user_builder.h"
#include "Game/controllers/action_panel_controller.h"
#include "ApplicationLayer/users/users_service.h"

class ActionPanelByUserBuilder::Private {
public:
	Private(ActionPanelByUserBuilder* parent) : q(parent) { }

	void addProfileButton() {
		QString cmd = QString("window-create user-profile %1")
			.arg(usersService->currentUserId());
		panel->addButton(ActionButtonConfig("Profile", "profile.png", "Профиль", cmd, 1));
	}

	void addChestButton() {
		QString cmd = QString("window-create inventory %1")
			.arg(usersService->getChestId().toString(QUuid::StringFormat::WithoutBraces));

		panel->addButton(ActionButtonConfig("Chest", "chest.png", "Сундук", cmd, 2));
	}

	void addEntitiesButton() {
		panel->addButton(ActionButtonConfig("Entities", "entities.png", "Библиотека предметов",
			"window-create item-entities item-entities", 2));
	}

	void addMapButton() {
		panel->addButton(ActionButtonConfig("Map", "map.png", "Карта мира",
			"window-create map 30f2aed5-5c42-4e6b-8179-f270a80159fe", 2));
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

	d->addProfileButton();
	d->addChestButton();
	d->addEntitiesButton();
	d->addMapButton();
}
