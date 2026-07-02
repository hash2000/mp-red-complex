#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

class CssHighlighterPlugin : public HighlightingPlugin {
public:
	~CssHighlighterPlugin() override;
	LanguageInfo languageInfo() const override;
	void install() override;
};
