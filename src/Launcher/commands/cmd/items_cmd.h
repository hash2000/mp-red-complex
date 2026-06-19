#pragma once
#include "Launcher/commands/command.h"
#include <memory>

class ItemsCommand : public CommandAbstraction {
	Q_OBJECT
public:
	ItemsCommand(QObject* parent = nullptr);
	~ItemsCommand() override;

	bool execute(CommandContext* context, const QStringList& args) override;

	QString name() const override { return "items-create"; }
	QString description() const override { return "Create new item in specified inventory"; }
	QString help() const override { return "items-create entity:entityId count:count inventory:inventoryId"; }

private:
	class Private;
	std::unique_ptr<Private> d;
};
