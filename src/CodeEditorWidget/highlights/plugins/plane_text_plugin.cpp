#include "CodeEditorWidget/highlights/plugins/plane_text_plugin.h"
#include "CodeEditorWidget/highlights/highlighter.h"

PlaneTextPlugin::LanguageInfo PlaneTextPlugin::languageInfo() const {
	return {
		"Text File",
		{ ".txt" },
		"text/plane",
		QIcon(":/icons/glsl.png")
	};
}

void PlaneTextPlugin::install(Highlighter& highlighter, HighlighterRuleType type) const {
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));
	highlighter.addRule("txt_numbers", "\\b[0-9]+\\b", numberFormat, 100, type);

	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый
	stringFormat.setFontWeight(QFont::Normal);
	highlighter.addRule("txt_strings", "\"[^\"]*\"", stringFormat, 100, type);
}

void PlaneTextPlugin::uninstall(Highlighter& highlighter) const {
	highlighter.deleteRule("txt_numbers");
	highlighter.deleteRule("txt_strings");
}

QStringList PlaneTextPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}
