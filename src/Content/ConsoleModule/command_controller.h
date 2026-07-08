#pragma once
#include "Libs/Engine/services/services_registry.h"

#include <QObject>
#include <QMap>
#include <memory>

class CommandProcessor;
class CommandContext;
class Resources;

class CommandController : public QObject {
	Q_OBJECT
public:
	CommandController(Resources* resources, QObject* parent = nullptr);
	~CommandController();

	void init();

	CommandProcessor* commandProcessor() const;
	CommandContext* commandContext() const;
	Resources* resources() const;

	bool execute(const QString& commandText, QObject* requester = nullptr);
	bool executeCommand(
		const QString& commandName,
		const QMap<QString, QString>& args,
		QObject* requester = nullptr);

	virtual std::unique_ptr<ServicesRegistry> createServices() = 0;

private:
	virtual void initContext() = 0;

signals:
	void commandExecuted(const QString& commandName);
	void commandFailed(const QString& commandName, const QString& errorMessage);

private:
	class Private;
	std::unique_ptr<Private> d;
};
