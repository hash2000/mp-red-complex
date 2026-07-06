#pragma once
#include "Launcher/commands/i_command.h"

class StatesStoreCommand : public ICommand {
	Q_OBJECT
public:
	StatesStoreCommand(QObject* parent = nullptr) : ICommand(parent) {}

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "states-store"; }
	QString description() const override { return "Serialize/deserialize application states"; }
	QString help() const override { return "states-store <'save' | 'load'>"; }
};
