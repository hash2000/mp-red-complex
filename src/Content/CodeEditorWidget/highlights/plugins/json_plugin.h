#pragma once
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

class JsonHighlighterPlugin : public HighlightingPlugin {
public:
	~JsonHighlighterPlugin() override;
	LanguageInfo languageInfo() const override;
	void install() override;
};
