#pragma once
#include <QList>
#include <QString>

/// Mime-типы переменных документа. По mime-типу диалог создания выбирает
/// редактор значения (текст, путь к файлу, число, флаг и т. п.).
namespace DocumentVariableMime {
inline constexpr const char* kText = "text/plain";
inline constexpr const char* kMultiline = "text/x-multiline";
inline constexpr const char* kFilePath = "text/x-file-path";
inline constexpr const char* kDirectoryPath = "text/x-directory-path";
inline constexpr const char* kBoolean = "application/x-boolean";
inline constexpr const char* kInteger = "application/x-integer";
inline constexpr const char* kNumber = "application/x-number";
inline constexpr const char* kColor = "application/x-color";
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
