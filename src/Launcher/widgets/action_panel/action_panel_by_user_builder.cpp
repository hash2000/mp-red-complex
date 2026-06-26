#include "Launcher/widgets/action_panel/action_panel_by_user_builder.h"
#include "Launcher/controllers/action_panel_controller.h"
#include "Content/UsersModule/services/users_service.h"
#include <QUuid>

class ActionPanelByUserBuilder::Private {
public:
	Private(ActionPanelByUserBuilder* parent) : q(parent) { }
	ActionPanelByUserBuilder* q;

	ActionPanelController* panel;
	UsersService* usersService;

	void addProfileButton() {
		QString cmd = QString("window-create target:user-profile id:%1")
			.arg(usersService->currentUserId());
		panel->addButton(ActionButtonConfig("Profile", "👤", "Профиль", cmd, 1));
	}

	void addChestButton() {
		//QString cmd = QString("window-create target:inventory id:%1")
		//	.arg(usersService->getChestId().toString(QUuid::StringFormat::WithoutBraces));

		//panel->addButton(ActionButtonConfig("Chest", "💼", "Сундук", cmd, 2));
	}

	void addEntitiesButton() {
		panel->addButton(ActionButtonConfig("Entities", "📚", "Библиотека предметов",
			"window-create target:item-entities id:item-entities", 2));
	}

	void addMapButton() {
		panel->addButton(ActionButtonConfig("Map", "🗺️", "Карта мира",
			"window-create target:map id:map", 2));
	}

	void addDefaultEditors() {
		panel->addButton(ActionButtonConfig("TextureEditor", "🎨", "Редактор текстур", "window-create target:texture-editor", 2));
		panel->addButton(ActionButtonConfig("MapEditor", "🗺️", "Редактор карты", "window-create target:map-editor", 2));
		panel->addButton(ActionButtonConfig("MaterialEditor", "🔥", "Редактор материалов", "window-create target:material-editor", 2));
		panel->addButton(ActionButtonConfig("CodeEditor", "🖹", "Редактор кода", "window-create target:code-editor", 2));
	}

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
	d->addDefaultEditors();
}
