#pragma once
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"

class JsonPlugin : public HighlightingPlugin {
public:
	~JsonPlugin() override;
	LanguageInfo languageInfo() const override;
	void install() override;
};
