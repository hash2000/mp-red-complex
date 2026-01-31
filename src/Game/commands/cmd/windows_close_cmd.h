#pragma once
#include "Game/commands/command.h"

class CloseWindowCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "windows-close"; }
	QString description() const override { return "Close window by ID or 'active'"; }
	QString help() const override { return "close <window_id | 'active' | 'all'> — close window"; }
	int minArgs() const override { return 1; }
};
