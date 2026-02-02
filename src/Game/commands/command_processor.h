#pragma once
#include <QObject>

class CommandContext;
class CommandResult;
class ICommand;

class CommandProcessor : public QObject {
Q_OBJECT
public:
  explicit CommandProcessor(QObject* parent = nullptr);
	~CommandProcessor() override;

public:
  // Регистрация команды (передача владения)
  void registerCommand(std::unique_ptr<ICommand> command);

  // Выполнение команды по тексту
  bool execute(const QString& commandLine, CommandContext* context);

  // Получение списка доступных команд
  QStringList availableCommands() const;

  // Получение команды по имени/алиасу
  ICommand* findCommand(const QString& name) const;

  // Справка по команде
  QString helpForCommand(const QString& name) const;
  QString fullHelp() const;

private:
	ICommand* findCommandUnsafe(const QString& name) const;
	bool executeCommand(ICommand* command, CommandContext* context, const QString& cmdName, const QStringList& args);

signals:
  void commandExecuted(const QString& commandName, bool success);
  void commandError(const QString& commandName, const QString& error);

private:
  class Private;
	std::unique_ptr<Private> d;
};
