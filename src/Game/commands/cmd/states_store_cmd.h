#pragma once
#include "Game/commands/command.h"

class StatesStoreCommand : public ICommand {
	Q_OBJECT
public:
	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "states-store"; }
	QString description() const override { return "Serialize/deserialize application states"; }
	QString help() const override { return "states-store — serialize/deserialize application states"; }
};
