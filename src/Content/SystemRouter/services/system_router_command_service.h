#pragma once
#include "Content/SystemRouter/data_providers/i_system_router_commands_provider.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QObject>
#include <QVariant>
#include <QStringList>

#include <memory>
#include <optional>

/// Сервис команд маршрутизатора.
///
/// Хранит таблицу команд и собирает скрипты узла: свойства с mime-типом
/// `application/x-mp-command` дают ссылку на команду, а свойства с
/// `application/x-mp-command-parameter` — значения для `{{параметр}}`.
class SystemRouterCommandService : public QObject {
	Q_OBJECT
public:
	explicit SystemRouterCommandService(ISystemRouterCommandsProvider* provider,
		QObject* parent = nullptr);
	~SystemRouterCommandService() override;

	ISystemRouterCommandsProvider* dataProvider() const;

	QList<SystemRouterCommand> commands() const;
	std::optional<SystemRouterCommand> command(const QString& id) const;
	bool saveCommand(const SystemRouterCommand& command);
	bool removeCommand(const QString& id);

	/// Все скрипты, привязанные к узлу, с уже подставленными параметрами.
	QList<SystemRouterScriptPreview> previewsForNode(const LazyTreeNodePtr& node) const;

	/// Подставить значения параметров узла в блоки `{{имя}}`.
	/// Для отсутствующего параметра вставляется `<<undefined>>`; его имя
	/// добавляется в missingParameters.
	QString renderScript(const QString& script, const LazyTreeNodePtr& node,
		QStringList* missingParameters = nullptr) const;

	/// Форматирует значение параметра в синтаксис ParserValue по его mime-типам.
	static QString formatParameterValue(const QStringList& mimeTypes, const QVariant& value);

	QString lastError() const;

signals:
	void errorOccurred(const QString& message);

private:
	class Private;
	std::unique_ptr<Private> d;
};
