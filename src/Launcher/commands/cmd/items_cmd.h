#pragma once
#include "Launcher/commands/command.h"
#include <memory>

class ItemsCommand : public CommandAbstraction {
	Q_OBJECT
public:
	ItemsCommand(QObject* parent = nullptr);
	~ItemsCommand() override;

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "items"; }
	QString description() const override { return "Operations with items"; }
	QString help() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
