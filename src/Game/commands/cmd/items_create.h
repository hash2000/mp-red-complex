#pragma once
#include "Game/commands/command.h"

class ItemsCreateCommand : public ICommand {
	Q_OBJECT
public:
	~ItemsCreateCommand() override;

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "items-create"; }
	QString description() const override { return "Create new item in specified inventory"; }
	QString help() const override { return "items-create entityId count inventoryId"; }
};
