#pragma once
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"

class MarkdownPlugin : public HighlightingPlugin {
public:
	~MarkdownPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
};
