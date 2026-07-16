#include "Launcher/windows/action_panel/action_panel_login_builder.h"
#include "Launcher/controllers/action_panel_controller.h"

ActionPanelLoginBuilder::ActionPanelLoginBuilder(ActionPanelController* panel)
	: _panel(panel) {
}

void ActionPanelLoginBuilder::build() {
	_panel->clearAllButtons();
	_panel->addButton(ActionButtonConfig("Login", "🔑", "Вход в систему",
		"windows action:create target:user-login id:login", 1));
	_panel->addButton(ActionButtonConfig("TextureEditor", "🎨", "Редактор текстур",
		"windows action:create target:texture-editor", 2));
	_panel->addButton(ActionButtonConfig("MapEditor", "🗺️", "Редактор карты",
		"windows action:create target:map-editor", 2));
	_panel->addButton(ActionButtonConfig("MaterialEditor", "🔥",
		"Редактор материалов", "windows action:create target:material-editor", 2));
	_panel->addButton(ActionButtonConfig("CodeEditor", "🖹", "Редактор кода",
		"windows action:create target:code-editor", 2));
	_panel->addButton(ActionButtonConfig("FetchApi", "📮", "Анализ запросов",
		"windows action:create target:fetch-api", 2));
}
