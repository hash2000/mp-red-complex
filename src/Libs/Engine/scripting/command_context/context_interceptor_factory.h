#pragma once
#include "Libs/Engine/scripting/command_context/document_rule.h"

#include <memory>

class IContextInterceptor;

/// Создаёт interceptor по роли документа.
///
/// Единственная точка расширения правил: новая роль — новый interceptor
/// в команде `command_context` и новая ветка здесь. Роль DocumentRule::Document
/// получает стандартный interceptor (текущее поведение системы).
std::unique_ptr<IContextInterceptor> createContextInterceptor(DocumentRule rule);
