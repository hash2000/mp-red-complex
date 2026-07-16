#pragma once
#include <QObject>
#include <QString>
#include <QMdiArea>
#include <memory>

class CommandController;
class ConsoleTable;
class ConsoleJson;
class ServicesRegistry;

constexpr const char* kCommandPrintStyle_Plane = "text/plain";
constexpr const char* kCommandPrintStyle_Table = "application/table";
constexpr const char* kCommandPrintStyle_Json = "application/json";

class CommandContext : public QObject {
	Q_OBJECT
public:
	explicit CommandContext(CommandController* commandController,
		CommandContext* globalContext,
		QObject* parent = nullptr);
	~CommandContext() override;

	// Доступ к основным компонентам приложения
	CommandController* commandController() const;
	CommandContext* globalContext();
	ServicesRegistry* services();

	bool isGlobalContext() const;

	std::unique_ptr<CommandContext> createScopedContext();

	// Вывод в консоль (безопасный из любого потока)
	void print(const QString& message, const QString& styleClass = "info", const QString& type = kCommandPrintStyle_Plane);
	void print(const ConsoleTable& table, const QString& styleClass = "info");
	void print(const ConsoleJson& json, const QString& styleClass = "info");
	void printSystem(const QString& message);
	void printError(const QString& message);
	void printSuccess(const QString& message);
	void printWarning(const QString& message);

signals:
	void outputRequested(const QString& message, const QString& styleClass, const QString& type);

private:
	class Private;
	std::unique_ptr<Private> d;
};
