#pragma once
#include "Libs/Engine/scripting/i_command.h"

/// Сохраняет содержимое code-editor в data_blobs.
///
/// Команда не принимает параметров: читает `blobId` и `data` из контекста
/// команды. Контекст создаётся от контекста code-editor, поэтому видны его
/// переменные.
class SystemRouterSaveBlobCommand final : public ICommand {
public:
	explicit SystemRouterSaveBlobCommand(QObject* parent = nullptr);
	~SystemRouterSaveBlobCommand() override;

	QString name() const override;
	QString description() const override;
	QString help() const override;

	bool execute(std::shared_ptr<CommandContext> context) override;
};
