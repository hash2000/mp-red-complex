#pragma once
#include <memory>

class ActionPanelController;
class UsersService;

class ActionPanelByUserBuilder {
public:
	ActionPanelByUserBuilder(ActionPanelController* panel, UsersService* usersService);

	virtual ~ActionPanelByUserBuilder();

	void build();

private:
	class Private;
	std::unique_ptr<Private> d;
};
