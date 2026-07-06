#pragma once
#include "Launcher/commands/i_command.h"

class HelpCommand : public ICommand {
public:
	HelpCommand(QObject* parent = nullptr) : ICommand(parent) { }

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "help"; }
	QString description() const override { return "Show help information"; }
	QString help() const override { return "help [command] — show help for command or all commands"; }
};
