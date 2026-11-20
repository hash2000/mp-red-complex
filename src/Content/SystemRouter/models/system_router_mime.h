#pragma once
#include "Libs/BaseWidgets/properties/property_mime.h"

/// Mime-типы маршрутизатора: связь узла с командой и параметры скрипта.
namespace SystemRouterMime {
inline constexpr const char* kCommand = "application/x-mp-command";
inline constexpr const char* kCommandParameter = "application/x-mp-command-parameter";
inline constexpr const char* kShared = PropertyMimeType::kShared;
inline constexpr const char* kContextVariable = PropertyMimeType::kContextVariable;

/// Свойство редактируется в code-editor: это параметр команды/контекста.
bool isCodeEditorProperty(const QStringList& mimeTypes);
} // namespace SystemRouterMime
