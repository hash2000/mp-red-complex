#pragma once
#include "Launcher/commands/command.h"

class ListWindowsCommand : public CommandAbstraction {
	Q_OBJECT
public:
	ListWindowsCommand(QObject* parent = nullptr) : CommandAbstraction(parent) {}

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "windows-list"; }
	QString description() const override { return "List all open MDI windows"; }
	QString help() const override { return "windows-list"; }
};
