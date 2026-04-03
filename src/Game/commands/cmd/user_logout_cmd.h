#pragma once
#include "Game/commands/command.h"

class UserLogoutCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "user-logout"; }
	QString description() const override { return "Logout"; }
	QString help() const override { return "user-logout"; }
};
