#pragma once
#include "Content/ConsoleModule/i_command.h"

class FetchApiCommand : public ICommand {
public:
	FetchApiCommand(QObject* parent = nullptr);
	~FetchApiCommand() override;

	bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;

	QString name() const override {
		return "fetch-api";
	}
	QString description() const override {
		return "Rest api queries sender";
	}

	QString help() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
