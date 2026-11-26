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

/// Глобальные параметры текстовых документов (редактор кода и виртуальный
/// редактор больших файлов). Значения по умолчанию применяются при создании
/// документа; документ может изменить их у себя, не трогая глобальные.
/// Предел буфера виртуального редактора в мегабайтах (0 — без предела).
constexpr const char* kDocumentsTextBufferLimit = "documents.text.buffer.limit";
constexpr int kDocumentsTextBufferLimitDefaultMb = 32;
/// Перенос длинных строк.
constexpr const char* kDocumentsTextWordWrap = "documents.text.wordwrap";
/// Размер шрифта в пунктах (0 — системный).
constexpr const char* kDocumentsTextFontSize = "documents.text.font.size";
/// Семейство шрифта (пустая строка — системный моноширинный).
constexpr const char* kDocumentsTextFontFamily = "documents.text.font.family";
/// Кодировка чтения и записи файла (имя кодека QTextCodec).
constexpr const char* kDocumentsTextEncoding = "documents.text.encoding";
constexpr const char* kDocumentsTextEncodingDefault = "UTF-8";
/// Размер отступа при нажатии Tab и ширина табуляции в символах.
constexpr const char* kDocumentsTextTabSpaces = "documents.text.tab.spaces";
constexpr int kDocumentsTextTabSpacesDefault = 2;
/// Стратегия Tab при наборе: spaces — пробелы, tabs — символ '\t'.
constexpr const char* kDocumentsTextTabEdit = "documents.text.tab.edit";
/// Стратегия сохранения ведущих табов: spaces — пробелы, tabs — как есть.
constexpr const char* kDocumentsTextTabSave = "documents.text.tab.save";
constexpr const char* kDocumentsTextTabModeSpaces = "spaces";
constexpr const char* kDocumentsTextTabModeTabs = "tabs";
/// Стратегия перемещения по токенам (Ctrl+Left/Right): идентификатор из
/// TextNavigationStrategyRegistry.
constexpr const char* kDocumentsTextNavigation = "documents.text.navigation";
constexpr const char* kDocumentsTextNavigationDefault = "words";
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
