#pragma once
#include "Libs/Engine/scripting/command_context/document_rule.h"

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <memory>

class CommandController;
class IContextInterceptor;
class ServicesRegistry;

class CommandContext : public QObject, public std::enable_shared_from_this<CommandContext> {
	Q_OBJECT
public:
	/// Как переменные текущего контекста соотносятся с переменными базового контекста.
	/// Режим — базовая политика хранения; правила роли документа
	/// (см. IContextInterceptor) имеют приоритет над ним.
	enum class VariablesMode {
		Lazy,      ///< читает из базового, при записи создаёт свою копию
		Mutable,   ///< читает и пишет общую переменную базового контекста
		ReadOnly,  ///< только чтение, запись запрещена
		Atomic,    ///< при переключении получает свою копию и дальше живёт отдельно
	};

	/// Правила контекста: приоритетнее VariablesMode (см. IContextInterceptor).
	/// Контекст без явных правил использует стандартный interceptor —
	/// текущее поведение системы.
	void setInterceptor(std::unique_ptr<IContextInterceptor> interceptor);
	const IContextInterceptor* interceptor() const;

	explicit CommandContext(CommandController* commandController, std::shared_ptr<CommandContext> parentContext, const QString& name, QObject* parent = nullptr);
	~CommandContext() override;

	// Доступ к основным компонентам приложения
	CommandController* commandController() const;

	QString name() const;
	QString nameSpace() const;
	/// Namespace зарегистрированных контекстов. `alive = true` (по умолчанию) —
	/// только живые; `false` включает записи, чьи владельцы уже уничтожены
	/// (диагностика и тесты очистки).
	QStringList nameSpaces(bool alive = true) const;

	bool isGlobalContext() const;
	std::shared_ptr<CommandContext> parentContext() const;
	std::shared_ptr<CommandContext> globalContext() const;
	std::shared_ptr<CommandContext> createScopedContext(const QString& name);
	std::shared_ptr<CommandContext> context(const QString& ns) const;
	/// Живые зарегистрированные прямые потомки; без передачи постоянного владения.
	/// Мёртвые записи (владелец уничтожил контекст) не возвращаются.
	QList<std::shared_ptr<CommandContext>> childContexts() const;

	// Переменные контекста. Имя может содержать точки: "wal.checkInterval".
	void setVariablesMode(VariablesMode mode);
	VariablesMode variablesMode() const;

	/// Запись переменной. false — контекст открыт только для чтения либо
	/// операцию запрещают правила контекста.
	bool setVariable(const QString& name, const QVariant& value);
	/// Удаление собственной переменной контекста. false — переменной нет,
	/// контекст открыт только для чтения либо удаление запрещено правилами.
	bool removeVariable(const QString& name);
	bool hasVariable(const QString& name) const;
	QVariant variable(const QString& name, const QVariant& defaultValue = QVariant()) const;
	/// Видимые переменные: базовые (с учётом режима) и собственные.
	QVariantMap variables() const;
	QStringList variableNames() const;
	/// Только собственные переменные контекста, без унаследованных.
	QVariantMap ownVariables() const;

	/// Mime-тип значения переменной (используется командами для разбора параметров).
	/// Несколько mime-типов в строковом виде разделяются запятыми.
	void setVariableMimeType(const QString& name, const QString& mimeType);
	QString variableMimeType(const QString& name, const QString& defaultValue = QString()) const;

	/// Mime-типы значения переменной: их может быть несколько (служебные
	/// пометки shared/context-variable вместе с типом значения). Пустой
	/// список снимает mime-тип.
	void setVariableMimeTypes(const QString& name, const QStringList& mimeTypes);
	/// Разобранный набор mime-типов переменной; пустой — mime-тип не задан.
	QStringList variableMimeTypes(const QString& name) const;

	// -------------------------------------------------------------------- события
	/// Отправить событие контекста: на него подписаны документы.
	void fireEvent(const QString& eventName);
	/// Имя события, которое отправляется по завершении пакета переменных.
	static const QString& variablesBatchCompleteEvent();
	/// Имя события, которое документ отправляет после успешного сохранения.
	static const QString& documentSaveCompleteEvent();

	// ------------------------------------------------- пакетное изменение переменных
	/// Начать пакетное изменение переменных: сигналы на отдельные переменные
	/// не рассылаются, а их имена копятся в changedVariables().
	void beginVariablesBatch();
	/// Завершить пакет: вместо отдельных сигналов отправляется событие
	/// `variables.batch.complete`.
	void endVariablesBatch();
	bool isVariablesBatchActive() const;
	/// Изменённые за пакет переменные. Вне пакета список пуст.
	QStringList changedVariables() const;

	// ---------------------------------------------------------------- документы
	/// Контекст документов, созданных этим контекстом (`{nameSpace}.documents`).
	/// Создаётся лениво и переиспользуется. Контекст открыт только для чтения:
	/// документы не могут менять системные параметры.
	std::shared_ptr<CommandContext> documentsContext();

	/// Корневой контекст системных настроек `.options`. Создаётся лениво
	/// один на приложение и всегда в глобальном контексте; режим — Mutable.
	std::shared_ptr<CommandContext> optionsContext();

	/// Контекст документа по его идентификатору среди документов этого контекста.
	std::shared_ptr<CommandContext> documentContext(const QString& documentId) const;

	/// Создаёт контекст документа по правилам роли DocumentRule.
	/// Обычный документ получает `{documents}.{documentId}`; документ
	/// системных настроек — корневой контекст `.options`.
	/// Повторный вызов для того же id возвращает существующий контекст.
	/// При создании в контексте документов испускается documentAttached().
	std::shared_ptr<CommandContext> createDocumentContext(const QString& documentId,
		DocumentRule rule = DocumentRule::Document);

	/// true, если этот контекст является контекстом документа
	/// (его родитель — контекст `documents`).
	bool isDocumentContext() const;
	/// Идентификатор документа (имя контекста) либо пустая строка.
	QString documentId() const;
	/// Ближайший контекст документа, в который вложен текущий (или сам текущий).
	/// Для контекста команды из документа это документ, вызвавший команду.
	std::shared_ptr<CommandContext> owningDocumentContext() const;

	/// Удаляет из дерева контекстов записи, которые уже никто не держит
	/// (владелец уничтожил контекст). Обход начинается от глобального
	/// контекста по всем вложенным. Возвращает количество удалённых записей.
	int pruneDeadContexts();

public slots:
	/// Уведомления о жизненном цикле документов.
	void notifyDocumentAttached(const QString& documentId);
	void notifyDocumentActivated(const QString& documentId);
	void notifyDocumentDeactivated();
	void notifyDocumentInvoked(const QString& documentId);

signals:
	/// Изменилось значение переменной контекста.
	void variableChanged(const QString& contextName, const QString& variableName,
		const QVariant& oldValue, const QVariant& newValue);

	/// Сработало событие контекста (см. fireEvent); документы следят за ним.
	void eventFired(const QString& eventName);

	/// В контексте документов появился документ (документ создал документ).
	void documentAttached(const QString& documentId);

	/// Документ стал активным (выбран вкладкой либо показан в зоне).
	void documentActivated(const QString& documentId);

	/// Документ перестал быть активным.
	void documentDeactivated();

	/// Другой документ вызвал этот документ (например, открыл панель свойств).
	void documentInvoked(const QString& documentId);

private:
	void registerContext(const QString& ns, std::weak_ptr<CommandContext> context);
	/// Чистит реестр этого контекста и рекурсивно — реестры живых детей.
	int pruneDeadSubtree();
	/// Корневой контекст по имени scope (`documents`, `options`, прочие).
	std::shared_ptr<CommandContext> namedScope(const QString& scopeName);

private:
	class Private;
	std::unique_ptr<Private> d;
};
