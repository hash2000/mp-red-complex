#pragma once
#include "Launcher/commands/command.h"

class CloseAllWindowsCommand : public CommandAbstraction {
	Q_OBJECT
public:
	CloseAllWindowsCommand(QObject* parent = nullptr) : CommandAbstraction(parent) {}

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "windows-closeall"; }
	QString description() const override { return "Close all open windows"; }
	QString help() const override { return "windows-closeall"; }
};
