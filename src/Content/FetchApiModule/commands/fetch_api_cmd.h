#pragma once
#include "Libs/Engine/scripting/i_command.h"

class FetchApiCommand : public ICommand {
public:
	FetchApiCommand(QObject* parent = nullptr);
	~FetchApiCommand() override;

	bool execute(std::shared_ptr<CommandContext> context) override;

	QString name() const override {
		return QStringLiteral("fetch-api");
	}
	QString description() const override {
		return QStringLiteral("Выполняет REST-запрос и пишет ответ в контекст");
	}

	QString help() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
