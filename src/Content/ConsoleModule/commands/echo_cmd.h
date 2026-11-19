#pragma once
#include "Libs/Engine/scripting/i_command.h"
#include <QString>

class EchoCommand : public ICommand {
public:
	EchoCommand(QObject* parent = nullptr) : ICommand(parent) { }

	bool execute(std::shared_ptr<CommandContext> context) override;

	QString name() const override { return "echo"; }
	QString description() const override { return "Print text"; }
	QString help() const override;
};
