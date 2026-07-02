#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

class MarkdownHighlighterPlugin : public HighlightingPlugin {
public:
	~MarkdownHighlighterPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
};
