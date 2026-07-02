#include "Content/CodeEditorWidget/highlights/plugins/plane_text_plugin.h"
#include "Content/CodeEditorWidget/language_info.h"

PlaneTextHighlighterPlugin::~PlaneTextHighlighterPlugin() = default;
LanguageInfo PlaneTextHighlighterPlugin::languageInfo() const {
	return {
		"Text File",
		{ ".txt" },
		"text/plane",
		QIcon(":/icons/glsl.png")
	};
}

void PlaneTextHighlighterPlugin::install()  {
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));
	addRule(Rule{ QRegularExpression("\\b[0-9]+\\b"), numberFormat, 100 });

	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый
	stringFormat.setFontWeight(QFont::Normal);
	addRule(Rule{ QRegularExpression("\"[^\"]*\""), stringFormat });
}

QStringList PlaneTextHighlighterPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}
