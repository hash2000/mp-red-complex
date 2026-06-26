#include "Content/CodeEditorWidget/highlights/plugins/plane_text_plugin.h"

PlaneTextPlugin::~PlaneTextPlugin() = default;

PlaneTextPlugin::LanguageInfo PlaneTextPlugin::languageInfo() const {
	return {
		"Text File",
		{ ".txt" },
		"text/plane",
		QIcon(":/icons/glsl.png")
	};
}

void PlaneTextPlugin::install()  {
	QTextCharFormat numberFormat;
	numberFormat.setForeground(QColor("#B5CEA8"));
	addRule(Rule{ QRegularExpression("\\b[0-9]+\\b"), numberFormat, 100 });

	QTextCharFormat stringFormat;
	stringFormat.setForeground(QColor("#CE9178"));  // оранжевый
	stringFormat.setFontWeight(QFont::Normal);
	addRule(Rule{ QRegularExpression("\"[^\"]*\""), stringFormat });
}

QStringList PlaneTextPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}
