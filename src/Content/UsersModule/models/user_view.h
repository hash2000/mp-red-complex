#pragma once
#include "Content/UsersModule/models/user.h"
#include <memory>
#include <set>

class UserView {
public:
	std::shared_ptr<UserData> data;
	std::set<QString> permissions;
	struct {		
		bool usersCreate = false;		// a user can create users
		bool usersView = false;			// a user can view all registered users
		bool userCanRegisterInGame = false;
		bool userCanRegisterInMessages = false;
		bool charactersView = false;
	} permissionsFlags;
};
