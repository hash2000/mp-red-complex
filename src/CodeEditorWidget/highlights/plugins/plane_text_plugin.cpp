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

void PlaneTextPlugin::install(Highlighter& highlighter) {
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));
	highlighter.addRule("numbers", "\\b[0-9]+\\b", numberFormat, 100);

	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый
	stringFormat.setFontWeight(QFont::Normal);
	highlighter.addRule("strings", "\"[^\"]*\"", stringFormat, 100);
}

void PlaneTextPlugin::uninstall(Highlighter& highlighter) {
	highlighter.deleteRule("numbers");
	highlighter.deleteRule("strings");
}

QStringList PlaneTextPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}
