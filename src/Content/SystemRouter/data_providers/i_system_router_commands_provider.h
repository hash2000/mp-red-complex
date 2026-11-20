#pragma once
#include "Content/SystemRouter/models/system_router_command.h"

#include <QList>
#include <QString>

#include <optional>

/// Поставщик команд маршрутизатора (таблица `commands`).
class ISystemRouterCommandsProvider {
public:
	virtual ~ISystemRouterCommandsProvider() = default;

	virtual QList<SystemRouterCommand> commands() const = 0;
	virtual std::optional<SystemRouterCommand> command(const QString& id) const = 0;
	virtual bool saveCommand(const SystemRouterCommand& command) = 0;
	virtual bool removeCommand(const QString& id) = 0;

	/// Описание последней ошибки; пустая строка — ошибки не было.
	virtual QString lastError() const {
		return {};
	}
};
