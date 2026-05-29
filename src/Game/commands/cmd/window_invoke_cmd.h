#pragma once
#include "Game/commands/command.h"

class WindowInvokeCommand : public ICommand {
public:
	explicit WindowInvokeCommand(QObject* parent = nullptr) : ICommand(parent) {}

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "window-invoke"; }
	QString description() const override { return "Invoke window handler"; }
	QString help() const override { return "window-invoke target:window[or active window] cmd:command [parameters that depend on the command being called]"; }
};
