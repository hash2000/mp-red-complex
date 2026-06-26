#pragma once
#include <QString>
#include <QStringList>

class IApplicationCommands {
public:
	/**
	 * Выполнение команды по текстовой строке
	 * @param commandText Полная строка команды (например: "focus editor1")
	 * @param requester Объект, инициировавший выполнение (для контекста)
	 * @return true если команда найдена и выполнена без исключений
	 */
	virtual bool executeCommand(const QString& commandText, QObject* requester = nullptr) = 0;

	/**
	 * Прямой вызов команды по имени с аргументами
	 */
	virtual bool executeCommandByName(const QString& commandName,
		const QStringList& args,
		QObject* requester = nullptr) = 0;
};
