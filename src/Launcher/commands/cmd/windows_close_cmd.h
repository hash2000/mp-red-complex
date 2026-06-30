#pragma once
#include "Launcher/commands/command.h"

class CloseWindowsCommand : public CommandAbstraction {
	Q_OBJECT
public:
	CloseWindowsCommand(QObject* parent = nullptr) : CommandAbstraction(parent) {}

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "window-close"; }
	QString description() const override { return "Close window by ID or 'active'"; }
	QString help() const override { return "window-close <window_id | 'active' | 'all'>"; }
	int minArgs() const override { return 1; }
};
