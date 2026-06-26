#pragma once
#include "Launcher/commands/command.h"

class HelpCommand : public CommandAbstraction {
public:
	HelpCommand(QObject* parent = nullptr) : CommandAbstraction(parent) { }

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "help"; }
	QString description() const override { return "Show help information"; }
	QString help() const override { return "help [command] — show help for command or all commands"; }
};
