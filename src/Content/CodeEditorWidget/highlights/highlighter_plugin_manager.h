#pragma once
#include <QString>
#include <memory>

class HighlightingPlugin;

class HighlightingPluginManager {
public:
	HighlightingPluginManager();
	~HighlightingPluginManager();

	void registerPlugin(std::unique_ptr<HighlightingPlugin> plugin);
	HighlightingPlugin* pluginForFile(const QString& filename) const;
	HighlightingPlugin* pluginForLanguage(const QString& name) const;
	HighlightingPlugin* pluginForContentType(const QString& name) const;
	QList<HighlightingPlugin*> allPlugins() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
