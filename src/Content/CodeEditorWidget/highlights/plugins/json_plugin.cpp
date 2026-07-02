#include "Content/CodeEditorWidget/highlights/plugins/json_plugin.h"
#include "Content/CodeEditorWidget/language_info.h"

#include <QRegularExpression>

JsonHighlighterPlugin::~JsonHighlighterPlugin() = default;
LanguageInfo JsonHighlighterPlugin::languageInfo() const {
	return { "JSON", {
		".json", ".jsonc", ".json5", ".geojson", ".har", ".jsonld" },
		"application/json" };
}

void JsonHighlighterPlugin::install() {
	// ==========================================
// 1. Строки (ключи и значения)
// ==========================================
	QTextCharFormat keyFormat;
	keyFormat.setForeground(QColor("#CE9178"));  // оранжевый
	keyFormat.setFontWeight(QFont::Bold);

	// Ключи объектов
	addRule({ QRegularExpression(R"("[^"]*"\s*:)"),
		keyFormat, 10 });

	// ==========================================
	// 2. Строковые значения
	// ==========================================
	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый

	addRule({ QRegularExpression(R"("[^"]*")"),
		stringFormat, 11 });

	// ==========================================
	// 3. Числа
	// ==========================================
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));  // светло-зеленый

	addRule({ QRegularExpression(R"(\b-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?\b)"),
		numberFormat, 12 });

	// ==========================================
	// 4. Булевы значения и null
	// ==========================================
	QTextCharFormat booleanFormat;
	booleanFormat.setForeground(QColor("#569CD6"));  // синий
	booleanFormat.setFontWeight(QFont::Bold);

	addRule({ QRegularExpression(R"(\b(?:true|false)\b)"),
		booleanFormat, 13 });

	QTextCharFormat nullFormat;
	nullFormat.setForeground(QColor("#569CD6"));  // синий
	nullFormat.setFontWeight(QFont::Bold);

	addRule({QRegularExpression(R"(\bnull\b)"),
		nullFormat, 13 });

	// ==========================================
	// 5. Структурные символы (опционально)
	// ==========================================
	QTextCharFormat structuralFormat;
	structuralFormat.setForeground(QColor("#D4D4D4"));  // светло-серый
	structuralFormat.setFontWeight(QFont::Bold);

	// Фигурные и квадратные скобки, двоеточия, запятые
	addRule({ QRegularExpression(R"([{}\[\]:,])"),
		structuralFormat, 30 });

	// ==========================================
	// 6. Комментарии (для JSONC и JSON5)
	// ==========================================
	QTextCharFormat commentFormat;
	commentFormat.setForeground(QColor("#6A9955"));  // зеленый
	commentFormat.setFontItalic(true);

	// Однострочные комментарии
	addRule({ QRegularExpression(R"(//[^\n]*)"),
		commentFormat, 5 });

	// Многострочные комментарии
	addRule({ QRegularExpression(R"(/\*[\s\S]*?\*/)"),
		commentFormat, 5 });

	// ==========================================
	// 7. Специальные значения JSON5
	// ==========================================
	QTextCharFormat specialFormat;
	specialFormat.setForeground(QColor("#D7BA7D"));  // золотой

	// NaN и Infinity в JSON5
	addRule({ QRegularExpression(R"(\b(?:NaN|Infinity|-Infinity)\b)"),
		specialFormat, 14 });

	// Шестнадцатеричные числа в JSON5
	addRule({ QRegularExpression(R"(\b0[xX][0-9a-fA-F]+\b)"),
		specialFormat, 14 });

	// ==========================================
	// 8. Escape-последовательности в строках
	// ==========================================
	QTextCharFormat escapeFormat;
	escapeFormat.setForeground(QColor("#D7BA7D"));  // золотой
	escapeFormat.setFontWeight(QFont::Bold);

	addRule({ QRegularExpression(R"(\\[\\"bfnrt/]|\\u[0-9a-fA-F]{4})"),
		escapeFormat, 9 });
}
