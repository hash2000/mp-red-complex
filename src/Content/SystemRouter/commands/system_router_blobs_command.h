#pragma once
#include "Libs/Engine/scripting/i_command.h"

/// Выводит в консоль таблицу со всеми блоками данных SystemRouter.
///
/// Команда не принимает параметров. Строки соответствуют записям `data_blobs`:
/// идентификатор, версия, описание и содержимое.
class SystemRouterBlobsCommand final : public ICommand {
public:
	explicit SystemRouterBlobsCommand(QObject* parent = nullptr);
	~SystemRouterBlobsCommand() override;

	QString name() const override;
	QString description() const override;
	QString help() const override;

	bool execute(std::shared_ptr<CommandContext> context) override;
};
