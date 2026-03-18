#pragma once
#include "Game/commands/command.h"

class CloseWindowsCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "window-close"; }
	QString description() const override { return "Close window by ID or 'active'"; }
	QString help() const override { return "window-close <window_id | 'active' | 'all'>"; }
	int minArgs() const override { return 1; }
};
