#pragma once
#include "CodeEditorWidget/highlights/plugins/highlighter_plugin.h"

class BashPlugin : public HighlightingPlugin {
public:
	~BashPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
	QStringList extractVariables(const QString& code) const override;
};
