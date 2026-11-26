#pragma once
#include "Libs/BaseWidgets/properties/properties_list_widget.h"

#include <QString>
#include <QStringList>
#include <QVariant>

/// Mime-типы свойств. Один mime-тип определяет редактор значения
/// (текст, число, цвет и т. п.); у свойства может быть несколько mime-типов.
namespace PropertyMimeType {
inline constexpr const char* kText = "text/plain";
inline constexpr const char* kMultiline = "text/x-multiline";
inline constexpr const char* kFilePath = "text/x-file-path";
inline constexpr const char* kDirectoryPath = "text/x-directory-path";
inline constexpr const char* kBoolean = "application/x-boolean";
inline constexpr const char* kInteger = "application/x-integer";
inline constexpr const char* kNumber = "application/x-number";
inline constexpr const char* kColor = "application/x-color";
inline constexpr const char* kJson = "application/json";
inline constexpr const char* kXml = "application/xml";
inline constexpr const char* kUriList = "text/uri-list";
/// Переменная, которую можно передать в контекст создаваемого документа.
inline constexpr const char* kShared = "application/x-mp-shared";
/// Значение свойства передаётся в CommandContext как переменная.
inline constexpr const char* kContextVariable = "application/x-mp-context-variable";
/// Секретные данные: свойство показывается, а его значение — нет.
inline constexpr const char* kSecret = "application/x-mp-secret";
/// Скрытое свойство: в документе просмотра оно не отображается вовсе.
inline constexpr const char* kHidden = "application/x-mp-hidden";
} // namespace PropertyMimeType

namespace PropertyMime {
/// Редактор значения по mime-типу свойства.
PropertyType editorType(const QString& mimeType);

/// Целочисленный mime-тип (для ограничений и нормализации значения).
bool isInteger(const QString& mimeType);

/// Основной mime-тип свойства: первый непустой из списка, иначе text/plain.
QString primaryMimeType(const QStringList& mimeTypes);

/// Mime-типы значения: убирает пустые строки и служебные пометки
/// (`shared`, `context variable`), которые не определяют редактор.
/// @param ignoredMimeTypes дополнительные служебные типы (например,
///        типы команд маршрутизатора)
QStringList valueMimeTypes(const QStringList& mimeTypes,
	const QStringList& ignoredMimeTypes = QStringList());

/// Основной mime-тип значения: первый после valueMimeTypes(), иначе text/plain.
QString primaryValueMimeType(const QStringList& mimeTypes,
	const QStringList& ignoredMimeTypes = QStringList());

/// Свойство помечено как секретное (`application/x-mp-secret`): его значение
/// не показывается в документах просмотра.
bool isSecret(const QStringList& mimeTypes);

/// Свойство помечено как скрытое (`application/x-mp-hidden`): в документе
/// просмотра оно не отображается.
bool isHidden(const QStringList& mimeTypes);

/// Распространённые mime-типы для диалога выбора (пользователь может
/// добавить произвольный).
QStringList commonMimeTypes();

/// Значение по умолчанию для mime-типа.
QVariant defaultValue(const QString& mimeType);

/// Приводит значение к типу, который ожидает редактор.
QVariant normalizeValue(const QVariant& value, const QString& mimeType);

/// Ограничения редактора числа: целые и дробные значения.
void applyNumberLimits(PropertyData& property, const QString& mimeType);
} // namespace PropertyMime
