#pragma once
#include <QObject>
#include <QString>
#include <QMdiArea>
#include <memory>

class CommandController;
class ServicesRegistry;

class CommandContext : public QObject , public std::enable_shared_from_this<CommandContext> {
	Q_OBJECT
public:
	explicit CommandContext(CommandController* commandController, std::shared_ptr<CommandContext> parentContext, const QString& name, QObject* parent = nullptr);
	~CommandContext() override;

	// Доступ к основным компонентам приложения
	CommandController* commandController() const;

	QString name() const;
	QString nameSpace() const;

	bool isGlobalContext() const;
	std::shared_ptr<CommandContext> parentContext() const;
	std::shared_ptr<CommandContext> globalContext() const;
	std::shared_ptr<CommandContext> createScopedContext(const QString& name);
	std::shared_ptr<CommandContext> context(const QString& ns) const;

private:
	void registerContext(const QString& ns, std::weak_ptr<CommandContext> context);

private:
	class Private;
	std::unique_ptr<Private> d;
};
