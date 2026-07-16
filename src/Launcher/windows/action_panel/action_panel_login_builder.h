#pragma once

class ActionPanelController;

class ActionPanelLoginBuilder {
public:
	ActionPanelLoginBuilder(ActionPanelController* panel);

	void build();

private:
	ActionPanelController* _panel;
};
