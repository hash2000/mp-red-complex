#pragma once
#include "CodeEditorWidget/highlights/plugins/i_highlighter_plugin.h"

class BashPlugin : public IHighlightingPlugin {
public:
	LanguageInfo languageInfo() const override;

	void install(Highlighter& highlighter) const override;
	void uninstall(Highlighter& highlighter) const override;
	QStringList extractVariables(const QString& code) const override;

	bool canHandleEmbeddedRegion(const QString& languageId) const override;

	void setupEmbeddedHighlighting(Highlighter& highlighter,
		const QString& languageId,
		const QMap<QString, QString>& metadata) const override;
};
