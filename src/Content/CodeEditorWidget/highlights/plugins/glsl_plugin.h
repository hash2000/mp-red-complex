#pragma once
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"

class GLSLPlugin : public HighlightingPlugin {
public:
	~GLSLPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
	QStringList extractVariables(const QString& code) const override;
};
