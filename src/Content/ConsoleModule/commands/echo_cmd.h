#pragma once
#include "Content/ConsoleModule/i_command.h"
#include <QString>

class EchoCommand : public ICommand {
public:
	EchoCommand(QObject* parent = nullptr) : ICommand(parent) { }

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "echo"; }
	QString description() const override { return "Print text"; }
	QString help() const override;
};
