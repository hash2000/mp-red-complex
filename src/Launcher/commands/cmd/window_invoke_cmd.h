#pragma once
#include "Launcher/commands/command.h"

class WindowInvokeCommand : public ICommand {
public:
	explicit WindowInvokeCommand(QObject* parent = nullptr) : ICommand(parent) {}

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "window-invoke"; }
	QString description() const override { return "Invoke window handler"; }
	QString help() const override;
};
