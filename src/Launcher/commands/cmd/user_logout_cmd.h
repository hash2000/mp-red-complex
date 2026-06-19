#pragma once
#include "Launcher/commands/command.h"

class UserLogoutCommand : public CommandAbstraction {
	Q_OBJECT
public:
	UserLogoutCommand(QObject* parent = nullptr) : CommandAbstraction(parent) {}

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "user-logout"; }
	QString description() const override { return "Logout"; }
	QString help() const override { return "user-logout"; }
};
