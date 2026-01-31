#pragma once
#include <QObject>
#include <QMdiArea>

class MdiChildWindow;
class ApplicationController;

class CommandContext : public QObject {
	Q_OBJECT
public:
	explicit CommandContext(ApplicationController* controller, QObject* parent = nullptr);
	~CommandContext() override;

	// Доступ к основным компонентам приложения
	ApplicationController* applicationController() const;
	QMdiArea* mdiArea() const;

	// Поиск активного/конкретного окна
	MdiChildWindow* activeWindow() const;
	MdiChildWindow* findWindowById(const QString& id) const;
	QList<MdiChildWindow*> allWindows() const;

	// Вывод в консоль (безопасный из любого потока)
	void print(const QString& message, const QString& styleClass = "info");
	void printError(const QString& message);
	void printSuccess(const QString& message);

	// Установка пользовательских данных для команд
	void setData(const QString& key, const QVariant& value);
	QVariant data(const QString& key) const;

signals:
	void outputRequested(const QString& message, const QString& styleClass);

private:
	class Private;
	std::unique_ptr<Private> d;
};
