#pragma once
#include "Game/commands/command.h"

class CreateWindowCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "window-create"; }
	QString description() const override { return "Create new MDI window by type"; }
	QString help() const override { return "window-create — Create new MDI window by type"; }
};
