#pragma once
#include <QVariant>
#include <QString>
#include <QStringList>

/// Имена системных параметров. Системный параметр — переменная глобального
/// контекста; описание параметра (подпись, тип, допустимые значения) хранит
/// SystemOptionsRegistry.
namespace SystemOptions {
/// Правило сортировки/группировки вкладок документов.
/// Значение — идентификатор правила из DocumentTabsOrderRegistry.
constexpr const char* kDocumentsTabsOrder = "documents.tabs.order";
} // namespace SystemOptions

/// Тип значения системного параметра: по нему выбирается редактор в UI.
enum class SystemOptionType {
	Text,
	Boolean,
	Integer,
	Color,
	Enum,
};

/// Описание системного параметра.
struct SystemOption {
	QString name;               ///< имя переменной глобального контекста
	QString title;              ///< подпись в интерфейсе настроек
	QString description;        ///< пояснение (tooltip)
	SystemOptionType type = SystemOptionType::Text;
	QVariant defaultValue;      ///< значение по умолчанию
	QStringList values;         ///< идентификаторы значений для Enum
	QStringList valueTitles;    ///< подписи значений для Enum (параллельно values)
};
