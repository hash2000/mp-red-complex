#pragma once
#include "Libs/BaseWidgets/properties/property_mime.h"

/// Mime-типы маршрутизатора: связь узла с командой и параметры скрипта.
namespace SystemRouterMime {
inline constexpr const char* kCommand = "application/x-mp-command";
inline constexpr const char* kCommandParameter = "application/x-mp-command-parameter";
inline constexpr const char* kShared = PropertyMimeType::kShared;
inline constexpr const char* kContextVariable = PropertyMimeType::kContextVariable;
/// Секретный параметр: в просмотре узла виден, значение — нет.
inline constexpr const char* kSecret = PropertyMimeType::kSecret;
/// Скрытый параметр: в просмотре узла не отображается.
inline constexpr const char* kHidden = PropertyMimeType::kHidden;

/// Свойство редактируется в code-editor: это параметр команды/контекста.
bool isCodeEditorProperty(const QStringList& mimeTypes);
} // namespace SystemRouterMime
