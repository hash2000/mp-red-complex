#pragma once
#include "Libs/BaseWidgets/properties/property_mime.h"

#include <QList>
#include <QString>

/// Mime-типы переменных документа. По mime-типу диалог создания выбирает
/// редактор значения (текст, путь к файлу, число, флаг и т. п.).
/// Константы общие с mime-типами свойств (Libs/BaseWidgets/properties).
namespace DocumentVariableMime {
inline constexpr const char* kText = PropertyMimeType::kText;
inline constexpr const char* kMultiline = PropertyMimeType::kMultiline;
inline constexpr const char* kFilePath = PropertyMimeType::kFilePath;
inline constexpr const char* kDirectoryPath = PropertyMimeType::kDirectoryPath;
inline constexpr const char* kBoolean = PropertyMimeType::kBoolean;
inline constexpr const char* kInteger = PropertyMimeType::kInteger;
inline constexpr const char* kNumber = PropertyMimeType::kNumber;
inline constexpr const char* kColor = PropertyMimeType::kColor;
inline constexpr const char* kShared = PropertyMimeType::kShared;
inline constexpr const char* kContextVariable = PropertyMimeType::kContextVariable;
} // namespace DocumentVariableMime

/// Обязательная переменная документа.
struct DocumentVariable {
	QString name;      ///< имя переменной контекста
	QString mimeType;  ///< mime-тип значения (см. DocumentVariableMime)
};

/// Метаданные типа документа: перечень обязательных переменных контекста.
/// Интерфейс (все методы виртуальные): реализация живёт рядом с документом
/// модуля (например, CodeEditorDocumentMetadata) и регистрируется в
/// WidgetRegistry по полному имени класса документа.
class IDocumentMetadata {
public:
	IDocumentMetadata() = default;
	virtual ~IDocumentMetadata() = default;

	/// Обязательные переменные документа и mime-типы их значений.
	virtual QList<DocumentVariable> variables() const = 0;
};
