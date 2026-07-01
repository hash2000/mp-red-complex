#pragma once
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"

class HtmlPlugin : public HighlightingPlugin {
public:
	~HtmlPlugin() override;
	LanguageInfo languageInfo() const override;
	void install() override;
};
