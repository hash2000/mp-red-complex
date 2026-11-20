#pragma once
#include "Content/SystemRouter/data_providers/i_system_router_commands_provider.h"

#include <memory>

class DatabasesService;

/// Поставщик команд SystemRouter в SQLite.
///
/// Таблица `commands`: id (GUID), version, description, script.
class SystemRouterCommandsProviderDb final : public ISystemRouterCommandsProvider {
public:
	explicit SystemRouterCommandsProviderDb(DatabasesService* databasesService);
	~SystemRouterCommandsProviderDb() override;

	QList<SystemRouterCommand> commands() const override;
	std::optional<SystemRouterCommand> command(const QString& id) const override;
	bool saveCommand(const SystemRouterCommand& command) override;
	bool removeCommand(const QString& id) override;
	QString lastError() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
