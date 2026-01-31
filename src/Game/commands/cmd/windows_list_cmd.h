#pragma once
#include "Game/commands/command.h"

class ListWindowsCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "windows-list"; }
	QString description() const override { return "List all open MDI windows"; }
	QString help() const override { return "windows — list all open windows"; }
};
