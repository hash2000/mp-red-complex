#include "Game/widgets/action_panel/action_panel_login_builder.h"
#include "Game/controllers/action_panel_controller.h"

ActionPanelLoginBuilder::ActionPanelLoginBuilder(ActionPanelController* panel)
	: _panel(panel) {
}

void ActionPanelLoginBuilder::build() {
	_panel->clearAllButtons();
	_panel->addButton(ActionButtonConfig("Login", "login.png", "Войти в систему", "window-create login login", 1));
}
