#include "CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "CodeEditorWidget/highlights/plugins/glsl_plugin.h"
#include "CodeEditorWidget/highlights/plugins/plane_text_plugin.h"
#include "CodeEditorWidget/highlights/plugins/markdown_plugin.h"
#include "CodeEditorWidget/highlights/plugins/bash_plugin.h"

#include <QFileInfo>
#include <map>


class HighlightingPluginManager::Private {
public:
	Private(HighlightingPluginManager* parent) : q(parent) {}
	HighlightingPluginManager* q;

	std::vector<std::unique_ptr<HighlightingPlugin>> plugins;
	std::map<QString, HighlightingPlugin*> extToPlugin;
};

HighlightingPluginManager::HighlightingPluginManager()
	: d(std::make_unique<Private>(this)) {
}

HighlightingPluginManager::~HighlightingPluginManager() = default;

void HighlightingPluginManager::loadPlugins(const QString& pluginsDir) {
	registerPlugin(std::make_unique<GLSLPlugin>());
	registerPlugin(std::make_unique<PlaneTextPlugin>());
	registerPlugin(std::make_unique<MarkdownPlugin>());
	registerPlugin(std::make_unique<BashPlugin>());
}

void HighlightingPluginManager::registerPlugin(std::unique_ptr<HighlightingPlugin> plugin) {
	const auto& info = plugin->languageInfo();
	for (const auto& ext : info.extensions) {
		d->extToPlugin[ext.toLower()] = plugin.get();
	}

	plugin->install();
	d->plugins.push_back(std::move(plugin));
}

HighlightingPlugin* HighlightingPluginManager::pluginForLanguage(const QString& name) const {
	const auto ext = name.startsWith(".") ? name : "." + name;
	auto it = d->extToPlugin.find(ext);
	return (it != d->extToPlugin.end()) ? it->second : nullptr;
}

HighlightingPlugin* HighlightingPluginManager::pluginForFile(const QString& filename) const {
	QFileInfo info(filename);
	QString ext = info.suffix().toLower();
	return pluginForLanguage(ext);
}

QList<HighlightingPlugin*> HighlightingPluginManager::allPlugins() const {
	QList<HighlightingPlugin*> result;
	for (const auto& plugin : d->plugins) {
		result.append(plugin.get());
	}
	return result;
}
