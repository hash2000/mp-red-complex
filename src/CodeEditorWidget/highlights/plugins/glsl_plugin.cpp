#include "CodeEditorWidget/highlights/plugins/glsl_plugin.h"
#include "CodeEditorWidget/highlights/plugins/glsl_plugin/highlighter_rule_glsl.h"
#include "CodeEditorWidget/highlights/highlighter.h"

GLSLPlugin::LanguageInfo GLSLPlugin::languageInfo() const {
	return {
		"GLSL Shader",
		{ ".vert", ".frag", ".geom", ".tesc", ".tese", ".comp", ".glsl" },
		"text/x-glsl",
		QIcon(":/icons/glsl.png")
	};
}

void GLSLPlugin::install(Highlighter& highlighter) {
	highlighter::rule::glsl::apply(highlighter);
}

void GLSLPlugin::uninstall(Highlighter& highlighter) {
	highlighter::rule::glsl::remove(highlighter);
}

QStringList GLSLPlugin::extractVariables(const QString& code) const {
	// GLSL-специфичный парсинг переменных
	return { };
}
