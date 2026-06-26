#pragma once
#include "Launcher/commands/command.h"

class WindowInvokeCommand : public CommandAbstraction {
public:
	WindowInvokeCommand(QObject* parent = nullptr) : CommandAbstraction(parent) {}

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "window-invoke"; }
	QString description() const override { return "Invoke window handler"; }
	QString help() const override;
};
