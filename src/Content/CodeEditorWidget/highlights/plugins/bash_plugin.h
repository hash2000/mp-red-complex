#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

class BashHighlighterPlugin : public HighlightingPlugin {
public:
	~BashHighlighterPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
	QStringList extractVariables(const QString& code) const override;
};
