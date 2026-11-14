#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <memory>

class CommandController;
class ServicesRegistry;

class CommandContext : public QObject, public std::enable_shared_from_this<CommandContext> {
	Q_OBJECT
public:
	/// Как переменные текущего контекста соотносятся с переменными базового контекста.
	enum class VariablesMode {
		Lazy,      ///< читает из базового, при записи создаёт свою копию
		Mutable,   ///< читает и пишет общую переменную базового контекста
		ReadOnly,  ///< только чтение, запись запрещена
		Atomic,    ///< при переключении получает свою копию и дальше живёт отдельно
	};

	explicit CommandContext(CommandController* commandController, std::shared_ptr<CommandContext> parentContext, const QString& name, QObject* parent = nullptr);
	~CommandContext() override;

	// Доступ к основным компонентам приложения
	CommandController* commandController() const;

	QString name() const;
	QString nameSpace() const;
	QStringList nameSpaces() const;

	bool isGlobalContext() const;
	std::shared_ptr<CommandContext> parentContext() const;
	std::shared_ptr<CommandContext> globalContext() const;
	std::shared_ptr<CommandContext> createScopedContext(const QString& name);
	std::shared_ptr<CommandContext> context(const QString& ns) const;

	// Переменные контекста. Имя может содержать точки: "wal.checkInterval".
	void setVariablesMode(VariablesMode mode);
	VariablesMode variablesMode() const;

	/// Запись переменной. false — контекст открыт только для чтения.
	bool setVariable(const QString& name, const QVariant& value);
	bool hasVariable(const QString& name) const;
	QVariant variable(const QString& name, const QVariant& defaultValue = QVariant()) const;
	/// Видимые переменные: базовые (с учётом режима) и собственные.
	QVariantMap variables() const;
	QStringList variableNames() const;
	/// Только собственные переменные контекста, без унаследованных.
	QVariantMap ownVariables() const;

	/// Mime-тип значения переменной (используется командами для разбора параметров).
	void setVariableMimeType(const QString& name, const QString& mimeType);
	QString variableMimeType(const QString& name, const QString& defaultValue = QString()) const;

private:
	void registerContext(const QString& ns, std::weak_ptr<CommandContext> context);
	void snapshotParentVariables();

private:
	class Private;
	std::unique_ptr<Private> d;
};
