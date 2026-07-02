#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

class GLSLHighlighterPlugin : public HighlightingPlugin {
public:
	~GLSLHighlighterPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
	QStringList extractVariables(const QString& code) const override;
};
