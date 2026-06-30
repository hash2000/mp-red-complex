#pragma once
#include "Launcher/commands/command.h"
#include <memory>

class CharactersCommand : public CommandAbstraction {
	Q_OBJECT
public:
	CharactersCommand(QObject* parent = nullptr);
	~CharactersCommand() override;

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override {
		return "characters";
	}
	QString description() const override {
		return "Operations with characters";
	}
	QString help() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
