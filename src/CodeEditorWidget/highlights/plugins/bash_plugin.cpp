#include "CodeEditorWidget/highlights/plugins/bash_plugin.h"
#include "CodeEditorWidget/highlights/highlighter.h"

BashPlugin::LanguageInfo BashPlugin::languageInfo() const {
	return { "Bash", {
		".sh", ".bash", ".bashrc", ".bash_profile", ".profile" },
		"text/x-sh",
		QIcon(":/icons/bash.png") };
}

void BashPlugin::install(Highlighter& highlighter) const {
	// ==========================================
	// 1. Комментарии
	// ==========================================
	QTextCharFormat commentFormat;
	commentFormat.setForeground(QColor("#6A9955"));  // зеленый
	commentFormat.setFontItalic(true);

	highlighter.addRule("bash_comment", "#[^\n]*", commentFormat, 5);

	// ==========================================
	// 2. Строки (двойные кавычки)
	// ==========================================
	QTextCharFormat doubleQuoteFormat;
	doubleQuoteFormat.setForeground(QColor("#CE9178"));  // оранжевый

	highlighter.addRule("bash_double_quotes", R"("[^"]*")", doubleQuoteFormat, 6);

	// ==========================================
	// 3. Строки (одинарные кавычки)
	// ==========================================
	QTextCharFormat singleQuoteFormat;
	singleQuoteFormat.setForeground(QColor("#CE9178"));  // оранжевый

	highlighter.addRule("bash_single_quotes", R"('[^']*')", singleQuoteFormat, 6);

	// ==========================================
	// 4. Ключевые слова
	// ==========================================
	QTextCharFormat keywordFormat;
	keywordFormat.setForeground(QColor("#569CD6"));  // синий
	keywordFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("bash_keywords",
		R"(\b(if|then|else|elif|fi|case|esac|for|while|until|do|done|in|select|function|time|coproc)\b)",
		keywordFormat, 10);

	// ==========================================
	// 5. Встроенные команды bash
	// ==========================================
	QTextCharFormat builtinFormat;
	builtinFormat.setForeground(QColor("#4EC9B0"));  // бирюзовый
	builtinFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("bash_builtins",
		R"(\b(alias|bg|bind|break|builtin|caller|cd|command|compgen|complete|compopt|)"
		R"(continue|declare|dirs|disown|echo|enable|eval|exec|exit|export|false|fc|)"
		R"(fg|getopts|hash|help|history|jobs|kill|let|local|logout|mapfile|popd|)"
		R"(printf|pushd|pwd|read|readarray|readonly|return|set|shift|shopt|source|)"
		R"(suspend|test|times|trap|true|type|typeset|ulimit|umask|unalias|unset|wait)\b)",
		builtinFormat, 11);

	// ==========================================
	// 6. Переменные
	// ==========================================
	QTextCharFormat variableFormat;
	variableFormat.setForeground(QColor("#9CDCFE"));  // голубой

	// Обычные переменные $var ${var}
	highlighter.addRule("bash_variables",
		R"(\$\{?\w+\}?)",
		variableFormat, 12);

	// Специальные переменные $0-$9, $#, $?, $$, $!, $@, $*
	highlighter.addRule("bash_special_vars",
		R"(\$[0-9#?!@*\-$])",
		variableFormat, 12);

	// ==========================================
	// 7. Числа
	// ==========================================
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));  // светло-зеленый

	highlighter.addRule("bash_numbers",
		R"(\b[0-9]+\.?[0-9]*(?:[eE][+-]?[0-9]+)?\b|)"
		R"(\b0[xX][0-9a-fA-F]+\b)",
		numberFormat, 7);

	// ==========================================
	// 8. Операторы и перенаправления
	// ==========================================
	QTextCharFormat operatorFormat;
	operatorFormat.setForeground(QColor("#D4D4D4"));  // светло-серый

	highlighter.addRule("bash_operators",
		R"([<>|&!]+|&&|\|\||>>|<<|>|>>|<|<<<|&>|>&|2>|2>>|2>&1|1>&2)",
		operatorFormat, 30);

	// ==========================================
	// 9. Тестовые операторы
	// ==========================================
	QTextCharFormat testFormat;
	testFormat.setForeground(QColor("#569CD6"));  // синий

	highlighter.addRule("bash_test_operators",
		R"(\b(-eq|-ne|-lt|-le|-gt|-ge|-z|-n|-f|-d|-e|-x|-r|-w|-s|-h|-L|-O|-G|-nt|-ot|-ef)\b)",
		testFormat, 13);

	// ==========================================
	// 10. Here-documents
	// ==========================================
	QTextCharFormat heredocFormat;
	heredocFormat.setForeground(QColor("#CE9178"));  // оранжевый

	highlighter.addRule("bash_heredoc_start",
		R"(<<[-]?\s*['"]?(\w+)['"]?)",
		heredocFormat, 15);

	// ==========================================
	// 11. Арифметические выражения
	// ==========================================
	QTextCharFormat arithFormat;
	arithFormat.setForeground(QColor("#FFE66D"));  // желтый

	highlighter.addRule("bash_arithmetic",
		R"(\$?\(\(.*\)\))",
		arithFormat, 14);

	// ==========================================
	// 12. Подстановки команд $(command) и `command`
	// ==========================================
	QTextCharFormat subFormat;
	subFormat.setForeground(QColor("#D7BA7D"));  // золотой

	highlighter.addRule("bash_command_sub",
		R"(\$\([^)]+\))",
		subFormat, 14);

	highlighter.addRule("bash_backtick_sub",
		R"(`[^`]+`)",
		subFormat, 14);

	// ==========================================
	// 13. Функции
	// ==========================================
	QTextCharFormat functionFormat;
	functionFormat.setForeground(QColor("#DCDCAA"));  // желтоватый
	functionFormat.setFontWeight(QFont::Bold);

	// function name() { }
	highlighter.addRule("bash_function_def",
		R"(^\s*(function\s+)?(\w+)\s*\(\s*\)\s*\{?)",
		functionFormat, 8);

	// ==========================================
	// 14. Shebang
	// ==========================================
	QTextCharFormat shebangFormat;
	shebangFormat.setForeground(QColor("#9B9B9B"));  // серый
	shebangFormat.setFontWeight(QFont::Bold);

	highlighter.addRule("bash_shebang",
		R"(^#!/.*$)",
		shebangFormat, 3);

	// ==========================================
	// 15. Escape-последовательности в строках
	// ==========================================
	QTextCharFormat escapeFormat;
	escapeFormat.setForeground(QColor("#D7BA7D"));  // золотой

	highlighter.addRule("bash_escape",
		R"(\\[\\nrtabfv\"'$])",
		escapeFormat, 4);

	// ==========================================
	// 16. Массивы
	// ==========================================
	QTextCharFormat arrayFormat;
	arrayFormat.setForeground(QColor("#9CDCFE"));  // голубой

	highlighter.addRule("bash_array",
		R"(\w+\s*=\s*\([^)]*\))",
		arrayFormat, 16);
}

void BashPlugin::uninstall(Highlighter& highlighter) const {
	highlighter.deleteRule("bash_comment");
	highlighter.deleteRule("bash_double_quotes");
	highlighter.deleteRule("bash_single_quotes");
	highlighter.deleteRule("bash_keywords");
	highlighter.deleteRule("bash_builtins");
	highlighter.deleteRule("bash_variables");
	highlighter.deleteRule("bash_special_vars");
	highlighter.deleteRule("bash_numbers");
	highlighter.deleteRule("bash_operators");
	highlighter.deleteRule("bash_test_operators");
	highlighter.deleteRule("bash_heredoc_start");
	highlighter.deleteRule("bash_arithmetic");
	highlighter.deleteRule("bash_command_sub");
	highlighter.deleteRule("bash_backtick_sub");
	highlighter.deleteRule("bash_function_def");
	highlighter.deleteRule("bash_shebang");
	highlighter.deleteRule("bash_escape");
	highlighter.deleteRule("bash_array");
}

QStringList BashPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}

bool BashPlugin::canHandleEmbeddedRegion(const QString& languageId) const {
	return languageId == "bash" || languageId == "cmd" ||
		languageId == "sh";
}

void BashPlugin::setupEmbeddedHighlighting(Highlighter& highlighter,
	const QString& languageId,
	const QMap<QString, QString>& metadata) const {
	install(highlighter);
}
