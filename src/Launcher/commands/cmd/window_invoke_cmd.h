#pragma once
#include "Launcher/commands/command.h"

class WindowInvokeCommand : public CommandAbstraction {
public:
	WindowInvokeCommand(QObject* parent = nullptr) : CommandAbstraction(parent) {}

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "window-invoke"; }
	QString description() const override { return "Invoke window handler"; }
	QString help() const override;
};
