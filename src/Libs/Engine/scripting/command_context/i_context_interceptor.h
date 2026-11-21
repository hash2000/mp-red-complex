#pragma once
#include "Libs/Engine/scripting/command_context.h"

#include <QString>
#include <QVariant>
#include <optional>

/// Правила, которые контекст применяет к операциям документа.
///
/// Проверки interceptor имеют приоритет над CommandContext::VariablesMode:
/// если правило запрещает операцию, режим переменных её уже не разрешит.
/// Роль документа создаёт свой interceptor (см. createContextInterceptor):
/// он определяет контекст, режим переменных и разрешённые операции.
class IContextInterceptor {
public:
	virtual ~IContextInterceptor() = default;

	/// Имя корневого контекста, в котором создаётся документ
	/// (`documents` → `.documents`, `options` → `.options`).
	virtual QString scopeName() const = 0;

	/// true — документ живёт прямо в контексте scope (`.options`),
	/// false — в дочернем контексте `.{scope}.{documentId}` (`.documents.{id}`).
	virtual bool documentLivesInScope() const = 0;

	/// Режим переменных контекста; nullopt — не менять текущий режим.
	virtual std::optional<CommandContext::VariablesMode> variablesMode() const = 0;

	/// Разрешено ли изменить (создать) переменную контекста.
	virtual bool canSetVariable(const CommandContext& context, const QString& name,
		const QVariant& value) const = 0;

	/// Разрешено ли удалить собственную переменную контекста.
	virtual bool canRemoveVariable(const CommandContext& context, const QString& name) const = 0;

	/// Разрешено ли документу инициировать событие контекста.
	virtual bool canFireEvent(const CommandContext& context, const QString& eventName) const = 0;
};
