#include "CodeEditorWidget/highlights/plugins/plane_text_plugin.h"
#include "CodeEditorWidget/highlights/plugins/plane_text_plugin/highlighter_rule_plane_text.h"
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
	highlighter::rule::plane_text::apply(highlighter);
}

void PlaneTextPlugin::uninstall(Highlighter& highlighter) {
	highlighter::rule::plane_text::remove(highlighter);
}

QStringList PlaneTextPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}
