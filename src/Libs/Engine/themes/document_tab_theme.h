#pragma once
#include <QColor>
#include <QHash>
#include <QString>

/// Цвета и стандартные глифы вкладок документов из темы.
///
/// Тема задаёт оформление вкладок в привязке к классу документа — файл
/// `assets/themes/{theme}/documents.yaml`:
///
/// ```yaml
/// tabs:
///   default: "#3c3c3c"         # необязательный цвет для всех вкладок
///   defaultGlyph: "📄"          # необязательный глиф для всех вкладок
///   colors:
///     Content.TextEditor.CodeEditor: "#61afef"  # полное имя класса
///     code-editor: "#61afef"                            # xtype
///   glyphs:
///     Content.TextEditor.CodeEditor: "📝"
///     code-editor: "📝"
/// ```
///
/// Ключ — полное имя класса (`WidgetTypeHandler::className`) или `xtype`.
/// У класса приоритет выше, чем у xtype; `default`/`defaultGlyph`
/// используются, если привязки для документа нет. Явные цвет и глиф
/// документа (`DocumentWidget::setColor`/`setGlyph`, панель свойств)
/// важнее значений темы.
class DocumentTabTheme {
public:
	/// Имя файла темы с оформлением вкладок.
	static const QString& fileName();

	/// Загружает тему из каталога; файла нет — тема пустая.
	static DocumentTabTheme loadFromThemeDirectory(const QString& themeDir);

	/// Загружает тему из файла; файл не читается — тема пустая.
	static DocumentTabTheme loadFromFile(const QString& filePath);

	/// Цвет вкладки документа: по классу, затем по xtype, затем default.
	QColor colorFor(const QString& className, const QString& xtype) const;

	/// Стандартный глиф вкладки: по классу, затем по xtype, затем defaultGlyph.
	QString glyphFor(const QString& className, const QString& xtype) const;

	/// Цвет для всех вкладок из секции `default` (может быть невалидным).
	QColor defaultColor() const;

	/// Глиф для всех вкладок из секции `defaultGlyph` (может быть пустым).
	QString defaultGlyph() const;

	/// Все привязки «класс или xtype → цвет» (диагностика и тесты).
	QHash<QString, QColor> colors() const;

	/// Все привязки «класс или xtype → глиф» (диагностика и тесты).
	QHash<QString, QString> glyphs() const;

	bool isEmpty() const;
	void clear();

private:
	QColor _defaultColor;
	QString _defaultGlyph;
	QHash<QString, QColor> _colors;
	QHash<QString, QString> _glyphs;
};
