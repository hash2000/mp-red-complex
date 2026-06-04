#include "CodeEditorWidget/highlights/plugins/bash_plugin.h"

#include <QRegularExpression>

BashPlugin::~BashPlugin() = default;

BashPlugin::LanguageInfo BashPlugin::languageInfo() const {
	return { "Bash", {
		".sh", ".bash", ".bashrc", ".bash_profile", ".profile" },
		"text/x-sh",
		QIcon(":/icons/bash.png") };
}

void BashPlugin::install() {
	// ==========================================
	// Комментарии
	// ==========================================
	QTextCharFormat commentFormat;
	commentFormat.setForeground(QColor("#6A9955"));  // зеленый
	commentFormat.setFontItalic(true);

	addRule(Rule{ QRegularExpression("#[^\n]*"), commentFormat, 5 });

	// ==========================================
	// Строки (двойные кавычки)
	// ==========================================
	QTextCharFormat doubleQuoteFormat;
	doubleQuoteFormat.setForeground(QColor("#CE9178"));  // оранжевый

	addRule(Rule{ QRegularExpression(R"("[^"]*")"), doubleQuoteFormat, 6 });

	// ==========================================
	// Строки (одинарные кавычки)
	// ==========================================
	QTextCharFormat singleQuoteFormat;
	singleQuoteFormat.setForeground(QColor("#CE9178"));  // оранжевый

	addRule(Rule{ QRegularExpression(R"('[^']*')"), singleQuoteFormat, 6 });

	// ==========================================
	// Ключевые слова
	// ==========================================
	QTextCharFormat keywordFormat;
	keywordFormat.setForeground(QColor("#569CD6"));  // синий
	keywordFormat.setFontWeight(QFont::Bold);

	addRule(Rule{ QRegularExpression(
		R"(\b(if|then|else|elif|fi|case|esac|for|while|until|do|done|in|select|function|time|coproc)\b)"),
		keywordFormat, 10 });

	// ==========================================
	// Встроенные команды bash
	// ==========================================
	QTextCharFormat builtinFormat;
	builtinFormat.setForeground(QColor("#4EC9B0"));  // бирюзовый
	builtinFormat.setFontWeight(QFont::Bold);

	addRule(Rule{ QRegularExpression(
		R"(\b(alias|bg|bind|break|builtin|caller|cd|command|compgen|complete|compopt|)"
		R"(continue|declare|dirs|disown|echo|enable|eval|exec|exit|export|false|fc|)"
		R"(fg|getopts|hash|help|history|jobs|kill|let|local|logout|mapfile|popd|)"
		R"(printf|pushd|pwd|read|readarray|readonly|return|set|shift|shopt|source|)"
		R"(suspend|test|times|trap|true|type|typeset|ulimit|umask|unalias|unset|wait)\b)"),
		builtinFormat, 11 });

	// ==========================================
	// Переменные
	// ==========================================
	QTextCharFormat variableFormat;
	variableFormat.setForeground(QColor("#9CDCFE"));  // голубой

	// Обычные переменные $var ${var}
	addRule(Rule{ QRegularExpression(
		R"(\$\{?\w+\}?)"),
		variableFormat, 12 });

	// Специальные переменные $0-$9, $#, $?, $$, $!, $@, $*
	addRule(Rule{ QRegularExpression(
		R"(\$[0-9#?!@*\-$])"),
		variableFormat, 12 });

	// ==========================================
	// Числа
	// ==========================================
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));  // светло-зеленый

	addRule(Rule{ QRegularExpression(
		R"(\b[0-9]+\.?[0-9]*(?:[eE][+-]?[0-9]+)?\b|)"
		R"(\b0[xX][0-9a-fA-F]+\b)"),
		numberFormat, 7 });

	// ==========================================
	// Операторы и перенаправления
	// ==========================================
	QTextCharFormat operatorFormat;
	operatorFormat.setForeground(QColor("#D4D4D4"));  // светло-серый

	addRule(Rule{ QRegularExpression(
		R"([<>|&!]+|&&|\|\||>>|<<|>|>>|<|<<<|&>|>&|2>|2>>|2>&1|1>&2)"),
		operatorFormat, 30 });

	// ==========================================
	// Тестовые операторы
	// ==========================================
	QTextCharFormat testFormat;
	testFormat.setForeground(QColor("#569CD6"));  // синий

	addRule(Rule{ QRegularExpression(
		R"(\b(-eq|-ne|-lt|-le|-gt|-ge|-z|-n|-f|-d|-e|-x|-r|-w|-s|-h|-L|-O|-G|-nt|-ot|-ef)\b)"),
		testFormat, 13 });

	// ==========================================
	// Here-documents
	// ==========================================
	QTextCharFormat heredocFormat;
	heredocFormat.setForeground(QColor("#CE9178"));  // оранжевый

	addRule(Rule{ QRegularExpression(
		R"(<<[-]?\s*['"]?(\w+)['"]?)"),
		heredocFormat, 15 });

	// ==========================================
	// Арифметические выражения
	// ==========================================
	QTextCharFormat arithFormat;
	arithFormat.setForeground(QColor("#FFE66D"));  // желтый

	addRule(Rule{ QRegularExpression(
		R"(\$?\(\(.*\)\))"),
		arithFormat, 14 });

	// ==========================================
	// Подстановки команд $(command) и `command`
	// ==========================================
	QTextCharFormat subFormat;
	subFormat.setForeground(QColor("#D7BA7D"));  // золотой

	addRule(Rule{ QRegularExpression(
		R"(\$\([^)]+\))"),
		subFormat, 14 });

	addRule(Rule{ QRegularExpression(
		R"(`[^`]+`)"),
		subFormat, 14 });

	// ==========================================
	// Функции
	// ==========================================
	QTextCharFormat functionFormat;
	functionFormat.setForeground(QColor("#DCDCAA"));  // желтоватый
	functionFormat.setFontWeight(QFont::Bold);

	// function name() { }
	addRule(Rule{ QRegularExpression(
		R"(^\s*(function\s+)?(\w+)\s*\(\s*\)\s*\{?)"),
		functionFormat, 8 });

	// ==========================================
	// Shebang
	// ==========================================
	QTextCharFormat shebangFormat;
	shebangFormat.setForeground(QColor("#9B9B9B"));  // серый
	shebangFormat.setFontWeight(QFont::Bold);

	addRule(Rule{ QRegularExpression(
		R"(^#!/.*$)"),
		shebangFormat, 3 });

	// ==========================================
	// Escape-последовательности в строках
	// ==========================================
	QTextCharFormat escapeFormat;
	escapeFormat.setForeground(QColor("#D7BA7D"));  // золотой

	addRule(Rule{ QRegularExpression(
		R"(\\[\\nrtabfv\"'$])"),
		escapeFormat, 4 });

	// ==========================================
	// Массивы
	// ==========================================
	QTextCharFormat arrayFormat;
	arrayFormat.setForeground(QColor("#9CDCFE"));  // голубой

	addRule(Rule{ QRegularExpression(
		R"(\w+\s*=\s*\([^)]*\))"),
		arrayFormat, 16 });
}

QStringList BashPlugin::extractVariables(const QString& code) const {
	return { };
}
