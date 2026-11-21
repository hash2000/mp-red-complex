#include "Libs/Engine/scripting/command_context/context_interceptor.h"

namespace {
/// Корневой контекст документов.
constexpr const char* kDocumentsScope = "documents";
/// Корневой контекст системных настроек.
constexpr const char* kOptionsScope = "options";
} // namespace

QString StandardContextInterceptor::scopeName() const {
	return QString::fromLatin1(kDocumentsScope);
}

bool StandardContextInterceptor::documentLivesInScope() const {
	return false;
}

std::optional<CommandContext::VariablesMode> StandardContextInterceptor::variablesMode() const {
	// Текущее поведение: режим документа не меняется (Lazy по умолчанию).
	return std::nullopt;
}

bool StandardContextInterceptor::canSetVariable(const CommandContext& context, const QString& name,
	const QVariant& value) const {
	Q_UNUSED(context);
	Q_UNUSED(name);
	Q_UNUSED(value);
	return true;
}

bool StandardContextInterceptor::canRemoveVariable(const CommandContext& context,
	const QString& name) const {
	Q_UNUSED(context);
	Q_UNUSED(name);
	return true;
}

bool StandardContextInterceptor::canFireEvent(const CommandContext& context,
	const QString& eventName) const {
	Q_UNUSED(context);
	Q_UNUSED(eventName);
	return true;
}

QString SystemOptionsContextInterceptor::scopeName() const {
	return QString::fromLatin1(kOptionsScope);
}

bool SystemOptionsContextInterceptor::documentLivesInScope() const {
	return true;
}

std::optional<CommandContext::VariablesMode> SystemOptionsContextInterceptor::variablesMode() const {
	return CommandContext::VariablesMode::Mutable;
}
