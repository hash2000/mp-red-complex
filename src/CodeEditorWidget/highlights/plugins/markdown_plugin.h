#pragma once
#include "CodeEditorWidget/highlights/plugins/i_highlighter_plugin.h"

class MarkdownPlugin : public IHighlightingPlugin {
public:
	LanguageInfo languageInfo() const override;

	void install(Highlighter& highlighter, HighlighterRuleType type) const override;
	void uninstall(Highlighter& highlighter) const override;
};
