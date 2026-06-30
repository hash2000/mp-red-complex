#pragma once
#include "Launcher/commands/command.h"

class StatesStoreCommand : public CommandAbstraction {
	Q_OBJECT
public:
	StatesStoreCommand(QObject* parent = nullptr) : CommandAbstraction(parent) {}

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override { return "states-store"; }
	QString description() const override { return "Serialize/deserialize application states"; }
	QString help() const override { return "states-store <'save' | 'load'>"; }
};
