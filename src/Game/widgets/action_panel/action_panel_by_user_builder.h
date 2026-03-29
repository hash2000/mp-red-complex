#pragma once

class ActionPanelController;

class ActionPanelByUserBuilder {
public:
	ActionPanelByUserBuilder(ActionPanelController* panel);

	void build();

private:
	ActionPanelController* _panel;
};
