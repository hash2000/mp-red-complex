#pragma once
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"

class PlaneTextPlugin : public HighlightingPlugin {
public:
	~PlaneTextPlugin() override;

	LanguageInfo languageInfo() const override;
	void install() override;
	QStringList extractVariables(const QString& code) const override;
};
