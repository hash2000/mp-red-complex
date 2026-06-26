#pragma once
#include "Launcher/commands/command.h"
#include <memory>

class UsersCommand : public CommandAbstraction {
	Q_OBJECT
public:
	UsersCommand(QObject* parent = nullptr);
	~UsersCommand() override;

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "users"; }
	QString description() const override { return "Operations with users"; }
	QString help() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
