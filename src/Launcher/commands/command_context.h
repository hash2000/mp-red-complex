#pragma once
#include <QObject>
#include <QString>
#include <QMdiArea>
#include <memory>

class MdiChildWindow;
class ApplicationController;
class MdiArea;
class Services;
class Controllers;
class ConsoleTable;

constexpr const char* kCommandPrintStyle_Plane = "text/plain";
constexpr const char* kCommandPrintStyle_Table = "application/table";

class CommandContext : public QObject {
	Q_OBJECT
public:
	explicit CommandContext(ApplicationController* applicationController,
		CommandContext* globalContext,
		QObject* parent = nullptr);
	~CommandContext() override;

	// Доступ к основным компонентам приложения
	ApplicationController* applicationController() const;
	Services* services() const;
	Controllers* controllers() const;
	CommandContext* globalContext();

	bool isGlobalContext() const;

	std::unique_ptr<CommandContext> createScopedContext();

	// Вывод в консоль (безопасный из любого потока)
	void print(const QString& message, const QString& styleClass = "info", const QString& type = kCommandPrintStyle_Plane);
	void print(const ConsoleTable& table, const QString& styleClass = "info");
	void printSystem(const QString& message);
	void printError(const QString& message);
	void printSuccess(const QString& message);
	void printWarning(const QString& message);

	// Установка пользовательских данных для команд
	void setData(const QString& key, const QVariant& value);
	QVariant data(const QString& key, const QVariant& def) const;

signals:
	void outputRequested(const QString& message, const QString& styleClass, const QString& type);

private:
	class Private;
	std::unique_ptr<Private> d;
};
