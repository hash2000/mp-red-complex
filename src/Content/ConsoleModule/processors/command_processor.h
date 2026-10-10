#pragma once
#include <QObject>
#include <memory>

class CommandContext;
class ICommand;
class Resources;

class CommandProcessor : public QObject {
Q_OBJECT
public:
  explicit CommandProcessor(Resources* resources, QObject* parent = nullptr);
	~CommandProcessor() override;

public:
  // Регистрация команды (передача владения)
  void registerCommand(std::unique_ptr<ICommand> command);

  // Выполнение команды по тексту
  bool execute(const QString& commandLine, std::shared_ptr<CommandContext> context);
	bool executeCommand(const QString& commandName, const QMap<QString, QString>& args, std::shared_ptr<CommandContext> context);

  // Получение списка доступных команд
  QStringList availableCommands() const;

  // Получение команды по имени/алиасу
	ICommand* findCommand(const QString& name) const;

signals:
  void commandExecuted(const QString& commandName, bool success);
  void commandError(const QString& commandName, const QString& error);

private:
  class Private;
	std::unique_ptr<Private> d;
};
