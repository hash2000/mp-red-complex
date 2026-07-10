#include "Content/CodeEditorWidget/highlights/plugins/html_plugin.h"
#include "Content/CodeEditorWidget/language_info.h"

#include <QRegularExpression>

HtmlHighlighterPlugin::~HtmlHighlighterPlugin() = default;
LanguageInfo HtmlHighlighterPlugin::languageInfo() const {
	return { "HTML", {
		".html", ".htm", ".xhtml", ".shtml" },
		"text/html" };
}

void HtmlHighlighterPlugin::install() {
	// ==========================================
	// 1. Комментарии
	// ==========================================
	QTextCharFormat commentFormat;
	commentFormat.setForeground(QColor("#6A9955"));  // зеленый
	commentFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression(R"(<!--[\s\S]*?-->)"), commentFormat, 5 });

	// ==========================================
	// 2. Теги (полные теги с атрибутами)
	// ==========================================
	QTextCharFormat tagFormat;
	tagFormat.setForeground(QColor("#569CD6"));  // синий
	tagFormat.setFontWeight(QFont::Bold);

	// Открывающие и закрывающие теги
	addRule(Rule{ QRegularExpression(R"(</?[\w\-]+[^>]*/?>)"), tagFormat, 10 });

	// Самозакрывающиеся теги
	addRule(Rule{ QRegularExpression(R"(<[\w\-]+[^>]*/>\s*)"), tagFormat, 10 });

	// ==========================================
	// 3. Имена тегов (для отдельной подсветки)
	// ==========================================
	QTextCharFormat tagNameFormat;
	tagNameFormat.setForeground(QColor("#569CD6"));  // синий
	tagNameFormat.setFontWeight(QFont::Bold);

	// Имя тега внутри скобок
	addRule(Rule{ QRegularExpression(R"((?<=</?)[\w\-]+(?=[\s>]))"), tagNameFormat, 11 });

	// ==========================================
	// 4. Атрибуты
	// ==========================================
	QTextCharFormat attributeFormat;
	attributeFormat.setForeground(QColor("#9CDCFE"));  // голубой

	// Имена атрибутов
	addRule(Rule{ QRegularExpression(R"((?<=\s)[\w\-]+(?=\s*=))"), attributeFormat, 15 });

	// ==========================================
	// 5. Значения атрибутов
	// ==========================================
	QTextCharFormat valueFormat;
	valueFormat.setForeground(QColor("#CE9178"));  // оранжевый

	// Значения в двойных кавычках
	addRule(Rule{ QRegularExpression(R"(=\s*"[^"]*")"), valueFormat, 16 });

	// Значения в одинарных кавычках
	addRule(Rule{ QRegularExpression(R"(=\s*'[^']*')"), valueFormat, 16 });

	// Значения без кавычек
	addRule(Rule{ QRegularExpression(R"(=\s*[\w\-]+)"), valueFormat, 16 });

	// ==========================================
	// 6. DOCTYPE декларация
	// ==========================================
	QTextCharFormat doctypeFormat;
	doctypeFormat.setForeground(QColor("#808080"));  // серый
	doctypeFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression(R"(<!DOCTYPE[\s\S]*?>)"), doctypeFormat, 8 });

	// ==========================================
	// 7. Специальные символы/entities
	// ==========================================
	QTextCharFormat entityFormat;
	entityFormat.setForeground(QColor("#D7BA7D"));  // золотой
	entityFormat.setFontWeight(QFont::Bold);

	addRule(Rule{ QRegularExpression(R"(&[a-zA-Z]+;|&#\d+;|&#x[0-9a-fA-F]+;)"), entityFormat, 20 });

	// ==========================================
	// 8. CDATA секции
	// ==========================================
	QTextCharFormat cdataFormat;
	cdataFormat.setForeground(QColor("#808080"));  // серый
	cdataFormat.setBackground(QColor("#2D2D2D"));

	addRule(Rule{ QRegularExpression(R"(<!\[CDATA\[[\s\S]*?\]\]>)"), cdataFormat, 7 });

	// ==========================================
	// 10. Условные комментарии IE
	// ==========================================
	QTextCharFormat conditionalFormat;
	conditionalFormat.setForeground(QColor("#808080"));
	conditionalFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression(R"(<!\[if[\s\S]*?<!\[endif\]\]>)"), conditionalFormat, 6 });

	// ==========================================
	// 11. PHP/Python/другие серверные вставки
	// ==========================================
	QTextCharFormat serverFormat;
	serverFormat.setForeground(QColor("#FF6B6B"));  // красный
	serverFormat.setBackground(QColor("#2D2D2D"));

	addRule(Rule{ QRegularExpression(R"(<\?[\s\S]*?\?>)"), serverFormat, 30 });

	// ==========================================
	// 12. Комментарии внутри тегов (IE conditional comments)
	// ==========================================
	QTextCharFormat ieCommentFormat;
	ieCommentFormat.setForeground(QColor("#6A9955"));
	ieCommentFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression(R"(<!--\[if[^\]]*\]>[\s\S]*?<!\[endif\]-->)"), ieCommentFormat, 6 });

	// Вложенный JavaScript в <script>
	QTextCharFormat embeddedJsFormat;
	embeddedJsFormat.setBackground(QColor("#1E1E1E"));

	addEmbeddedBlockRule(EmbeddedBlockRule{
		QRegularExpression(R"(<script[^>]*\btype\s*=\s*['"]application/json['"][^>]*>)", QRegularExpression::CaseInsensitiveOption),
		QRegularExpression(R"(</script>)", QRegularExpression::CaseInsensitiveOption),
		"json", // Фиксированный язык
		embeddedJsFormat,
		25 });

	// Вложенный CSS в <style>
	QTextCharFormat embeddedCssFormat;
	embeddedCssFormat.setBackground(QColor("#1E1E1E"));

	addEmbeddedBlockRule(EmbeddedBlockRule{
		QRegularExpression(R"(<style[^>]*>)"),     // Начало: <style>
		QRegularExpression(R"(</style>)"),         // Конец: </style>
		"css",                 // Фиксированный язык
		embeddedCssFormat,
		25 });
}
