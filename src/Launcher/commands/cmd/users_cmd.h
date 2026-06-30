#pragma once
#include "Launcher/commands/command.h"
#include <memory>

class UsersCommand : public CommandAbstraction {
	Q_OBJECT
public:
	UsersCommand(QObject* parent = nullptr);
	~UsersCommand() override;

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "users"; }
	QString description() const override { return "Operations with users"; }
	QString help() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
