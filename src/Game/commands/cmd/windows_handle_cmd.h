#pragma once
#include "Game/commands/command.h"

class HandleWindowsCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "windows-handle"; }
	QString description() const override { return "Handle commend to the specified window"; }
	QString help() const override { return "windows — handle command"; }
};
