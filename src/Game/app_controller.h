#pragma once
#include <QObject>
#include <QPointer>
#include <QHash>
#include <QPair>
#include <QList>
#include <memory>

class CommandProcessor;
class CommandContext;
class WindowsController;

class ApplicationController : public QObject {
Q_OBJECT
public:
	ApplicationController(QObject* parent = nullptr);
	~ApplicationController();

	CommandProcessor* commandProcessor() const;
	CommandContext* commandContext() const;
	WindowsController* windowsController() const;

	/**
	 * Выполнение команды по текстовой строке
	 * @param commandText Полная строка команды (например: "focus editor1")
	 * @param requester Объект, инициировавший выполнение (для контекста)
	 * @return true если команда найдена и выполнена без исключений
	 */
	bool executeCommand(const QString& commandText, QObject* requester = nullptr);

	/**
	 * Прямой вызов команды по имени с аргументами
	 */
	bool executeCommandByName(const QString& commandName,
		const QStringList& args,
		QObject* requester = nullptr);

signals:
	/**
	 * Сигнал об успешном выполнении команды
	 * @param commandName Имя команды
	 * @param executionTimeMs Время выполнения в миллисекундах
	 */
	void commandExecuted(const QString& commandName, qint64 executionTimeMs);

	/**
	 * Сигнал об ошибке выполнения команды
	 * @param commandName Имя команды
	 * @param errorMessage Текст ошибки
	 */
	void commandFailed(const QString& commandName, const QString& errorMessage);

private:
	class Private;
	std::unique_ptr<Private> d;
};
