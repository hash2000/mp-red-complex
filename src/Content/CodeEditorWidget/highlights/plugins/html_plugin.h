#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

class HtmlHighlighterPlugin : public HighlightingPlugin {
public:
	~HtmlHighlighterPlugin() override;
	LanguageInfo languageInfo() const override;
	void install() override;
};
