#pragma once
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"

class CssPlugin : public HighlightingPlugin {
public:
	~CssPlugin() override;
	LanguageInfo languageInfo() const override;
	void install() override;
};
