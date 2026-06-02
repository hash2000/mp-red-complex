#pragma once
#include <QString>
#include <memory>

class IHighlightingPlugin;

class HighlightingPluginManager {
public:
	HighlightingPluginManager();
	~HighlightingPluginManager();

	void loadPlugins(const QString& pluginsDir);
	void registerPlugin(std::unique_ptr<IHighlightingPlugin> plugin);
	IHighlightingPlugin* pluginForFile(const QString& filename) const;
	IHighlightingPlugin* pluginForLanguage(const QString& name) const;
	QList<IHighlightingPlugin*> allPlugins() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
