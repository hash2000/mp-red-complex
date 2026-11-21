#pragma once
#include "Libs/Engine/scripting/command_context/i_context_interceptor.h"

/// Стандартные правила контекста — текущее поведение системы.
/// Документ создаётся в `.documents.{id}`, режим переменных не меняется
/// (документ работает как Lazy), переменные и события разрешены.
class StandardContextInterceptor : public IContextInterceptor {
public:
	QString scopeName() const override;
	bool documentLivesInScope() const override;
	std::optional<CommandContext::VariablesMode> variablesMode() const override;

	bool canSetVariable(const CommandContext& context, const QString& name,
		const QVariant& value) const override;
	bool canRemoveVariable(const CommandContext& context, const QString& name) const override;
	bool canFireEvent(const CommandContext& context, const QString& eventName) const override;
};

/// Правила документа системных настроек: документ создаётся в контексте
/// `.options` и работает в режиме Mutable, поэтому его записи попадают
/// в системные параметры глобального контекста.
class SystemOptionsContextInterceptor final : public StandardContextInterceptor {
public:
	QString scopeName() const override;
	bool documentLivesInScope() const override;
	std::optional<CommandContext::VariablesMode> variablesMode() const override;
};
