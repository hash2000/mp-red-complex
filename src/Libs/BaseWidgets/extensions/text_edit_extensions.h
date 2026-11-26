#pragma once
#include <QPlainTextEdit>
#include <QString>
#include <QStringList>
#include <QTextEdit>

/// Общие настройки текстовых редакторов: ширина табуляции и стратегии
/// отступов (аналог `tabWidth`/`useTabs` в Prettier).
namespace Extensions::TextEdit {

/// Как набирать и сохранять отступы.
enum class TabMode {
	Spaces, ///< отступ пробелами
	Tabs,   ///< отступ символом '\t'
};

/// Ширина табуляции в символах (не меньше одного).
int normalizedTabWidth(int spaces);

/// Задать расстояние табуляции у QTextEdit/QPlainTextEdit (в пикселях,
/// по ширине моноширинного символа). nullptr допустим.
QTextEdit* setTabDistance(QTextEdit* obj, int distance);
QPlainTextEdit* setTabDistance(QPlainTextEdit* obj, int distance);

/// Идентификаторы/подписи стратегий для системных параметров.
QStringList tabModeIds();
QStringList tabModeTitles();

TabMode tabModeFromString(const QString& value, TabMode fallback = TabMode::Spaces);
QString tabModeToString(TabMode mode);

/// Один отступ: `spaces` пробелов либо один '\t'.
QString indentText(int spaces, TabMode mode);

/// Заменить ведущие табы каждой строки пробелами (по `spaces`).
/// Табы после первого непробельного символа не трогаются: это отступы,
/// а не содержимое строки.
QString convertLeadingTabsToSpaces(const QString& text, int spaces);

/// Текст для записи в файл с учётом стратегии сохранения.
QString textForSave(const QString& text, int spaces, TabMode mode);

} // namespace Extensions::TextEdit
