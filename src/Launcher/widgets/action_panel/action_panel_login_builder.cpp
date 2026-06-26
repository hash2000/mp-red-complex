#include "Launcher/widgets/action_panel/action_panel_login_builder.h"
#include "Launcher/controllers/action_panel_controller.h"

ActionPanelLoginBuilder::ActionPanelLoginBuilder(ActionPanelController* panel)
	: _panel(panel) {
}

void ActionPanelLoginBuilder::build() {
	_panel->clearAllButtons();
	_panel->addButton(ActionButtonConfig("Login", "🔑", "Вход в систему", "window-create target:user-login id:login", 1));
	_panel->addButton(ActionButtonConfig("TextureEditor", "🎨", "Редактор текстур", "window-create target:texture-editor", 2));
	_panel->addButton(ActionButtonConfig("MapEditor", "🗺️", "Редактор карты", "window-create target:map-editor", 2));
	_panel->addButton(ActionButtonConfig("MaterialEditor", "🔥", "Редактор материалов", "window-create target:material-editor", 2));
	_panel->addButton(ActionButtonConfig("CodeEditor", "🖹", "Редактор кода", "window-create target:code-editor", 2));
}
