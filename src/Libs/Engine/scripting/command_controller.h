#pragma once

#include <QObject>
#include <QMap>
#include <memory>

class CommandProcessor;
class CommandContext;
class ServicesRegistry;
class WidgetRegistry;
class MainWindow;
class Resources;
class ConsoleTable;
class ConsoleJson;

constexpr const char* kCommandPrintStyle_Plane = "text/plain";
constexpr const char* kCommandPrintStyle_Table = "application/table";
constexpr const char* kCommandPrintStyle_Json = "application/json";

class CommandController : public QObject {
	Q_OBJECT
public:
	CommandController(Resources* resources, QObject* parent = nullptr);
	~CommandController();

	void init();

	virtual CommandProcessor* commandProcessor() const = 0;
	virtual ServicesRegistry* services() const = 0;
	WidgetRegistry* widgets() const;
	Resources* resources() const;

	/// Активная оболочка; указатель не владеет окном и может быть nullptr.
	MainWindow* mainWindow() const;
	void setMainWindow(MainWindow* window);

	std::shared_ptr<CommandContext> globalContext();

	bool execute(const QString& commandText, QObject* requester = nullptr);
	bool executeCommand(const QString& commandName, const QMap<QString, QString>& args, QObject* requester = nullptr);

	// Вывод в консоль (безопасный из любого потока)
	void print(const QString& message, const QString& styleClass = "info", const QString& type = kCommandPrintStyle_Plane);
	void print(const ConsoleTable& table, const QString& styleClass = "info");
	void print(const ConsoleJson& json, const QString& styleClass = "info");
	void printSystem(const QString& message);
	void printError(const QString& message);
	void printSuccess(const QString& message);
	void printWarning(const QString& message);

private:
	virtual void initContext() = 0;

signals:
	void outputRequested(const QString& message, const QString& styleClass, const QString& type);

signals:
	void commandExecuted(const QString& commandName);
	void commandFailed(const QString& commandName, const QString& errorMessage);

private:
	class Private;
	std::unique_ptr<Private> d;
};
