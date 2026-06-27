#pragma once
#include "Launcher/commands/command.h"

class FetchApiCommand : public CommandAbstraction {
public:
	FetchApiCommand(QObject* parent = nullptr);
	~FetchApiCommand() override;

	bool execute(CommandContext* context, const QStringList& args) override;

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
