#pragma once
#include "Launcher/i_app_commands.h"
#include <QObject>
#include <QPointer>
#include <QMap>
#include <QPair>
#include <QList>
#include <memory>

class CommandProcessor;
class CommandContext;
class EventBus;
class Resources;

class ApplicationController : public QObject, public IApplicationCommands {
Q_OBJECT
public:
	ApplicationController(Resources* resources, QObject* parent = nullptr);
	~ApplicationController();

	CommandProcessor* commandProcessor() const;
	CommandContext* commandContext() const;
	Resources* resources() const;

	bool execute(const QString& commandText, QObject* requester = nullptr) override;

	bool executeCommand(const QString& commandName, const QMap<QString, QString>& args,
		QObject* requester = nullptr);

signals:
	void commandExecuted(const QString& commandName);
	void commandFailed(const QString& commandName, const QString& errorMessage);

private:
	class Private;
	std::unique_ptr<Private> d;
};
