#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

class PlaneTextHighlighterPlugin : public HighlightingPlugin {
public:
	~PlaneTextHighlighterPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
	QStringList extractVariables(const QString& code) const override;
};
