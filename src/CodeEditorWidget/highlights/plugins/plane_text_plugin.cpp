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

void PlaneTextPlugin::install(Highlighter& highlighter) const {
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));
	highlighter.addRule("numbers", "\\b[0-9]+\\b", numberFormat, 100);

	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый
	stringFormat.setFontWeight(QFont::Normal);
	highlighter.addRule("strings", "\"[^\"]*\"", stringFormat, 100);
}

void PlaneTextPlugin::uninstall(Highlighter& highlighter) const {
	highlighter.deleteRule("numbers");
	highlighter.deleteRule("strings");
}

QStringList PlaneTextPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}

bool PlaneTextPlugin::canHandleEmbeddedRegion(const QString& languageId) const {
	return languageId == "text" || languageId == "txt";
}

void PlaneTextPlugin::setupEmbeddedHighlighting(Highlighter& highlighter,
	const QString& languageId,
	const QMap<QString, QString>& metadata) const {
	install(highlighter);
}
