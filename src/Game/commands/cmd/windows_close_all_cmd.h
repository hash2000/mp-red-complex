#pragma once
#include "Game/commands/command.h"

class CloseAllWindowsCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "windows-closeall"; }
	QString description() const override { return "Close all open windows"; }
	QString help() const override { return "closeall — close all windows"; }
};
