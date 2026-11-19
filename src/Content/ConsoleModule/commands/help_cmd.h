#pragma once
#include "Libs/Engine/scripting/i_command.h"
#include <QString>

class HelpCommand : public ICommand {
public:
	HelpCommand(QObject* parent = nullptr) : ICommand(parent) { }

	bool execute(std::shared_ptr<CommandContext> context) override;

	QString name() const override { return "help"; }
	QString description() const override { return "Show help information"; }
	QString help() const override { return "help — show all commands; help name=<command|variable> — show one command or variable"; }
};
