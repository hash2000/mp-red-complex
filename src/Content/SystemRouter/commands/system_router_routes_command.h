#pragma once
#include "Libs/Engine/scripting/i_command.h"

/// Выводит в консоль таблицу со всеми узлами SystemRouter (`nodes`).
///
/// Команда не принимает параметров. Таблица содержит id, parent_id, name
/// и data — JSON-представление свойств узла.
class SystemRouterRoutesCommand final : public ICommand {
public:
	explicit SystemRouterRoutesCommand(QObject* parent = nullptr);
	~SystemRouterRoutesCommand() override;

	QString name() const override;
	QString description() const override;
	QString help() const override;

	bool execute(std::shared_ptr<CommandContext> context) override;
};
