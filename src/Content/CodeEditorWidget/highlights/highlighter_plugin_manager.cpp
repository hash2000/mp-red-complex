#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/plugins/glsl_plugin.h"
#include "Content/CodeEditorWidget/highlights/plugins/plane_text_plugin.h"
#include "Content/CodeEditorWidget/highlights/plugins/markdown_plugin.h"
#include "Content/CodeEditorWidget/highlights/plugins/bash_plugin.h"
#include "Content/CodeEditorWidget/highlights/plugins/html_plugin.h"
#include "Content/CodeEditorWidget/highlights/plugins/css_plugin.h"
#include "Content/CodeEditorWidget/highlights/plugins/json_plugin.h"

#include <QFileInfo>
#include <map>


class HighlightingPluginManager::Private {
public:
	Private(HighlightingPluginManager* parent) : q(parent) {}
	HighlightingPluginManager* q;

	std::vector<std::unique_ptr<HighlightingPlugin>> plugins;
	std::map<QString, HighlightingPlugin*> extToPlugin;
	std::map<QString, HighlightingPlugin*> contentTypeToPlugin;
};

HighlightingPluginManager::HighlightingPluginManager()
	: d(std::make_unique<Private>(this)) {
}

HighlightingPluginManager::~HighlightingPluginManager() = default;

void HighlightingPluginManager::loadPlugins(const QString& pluginsDir) {
	registerPlugin(std::make_unique<GLSLHighlighterPlugin>());
	registerPlugin(std::make_unique<PlaneTextHighlighterPlugin>());
	registerPlugin(std::make_unique<MarkdownHighlighterPlugin>());
	registerPlugin(std::make_unique<BashHighlighterPlugin>());
	registerPlugin(std::make_unique<HtmlHighlighterPlugin>());
	registerPlugin(std::make_unique<CssHighlighterPlugin>());
	registerPlugin(std::make_unique<JsonHighlighterPlugin>());
}

void HighlightingPluginManager::registerPlugin(std::unique_ptr<HighlightingPlugin> plugin) {
	const auto& info = plugin->languageInfo();
	for (const auto& ext : info.extensions) {
		d->extToPlugin[ext.toLower()] = plugin.get();
	}

	if (!info.mimeType.isEmpty()) {
		d->contentTypeToPlugin[info.mimeType.toLower()] = plugin.get();
	}

	plugin->install();
	d->plugins.push_back(std::move(plugin));
}

HighlightingPlugin* HighlightingPluginManager::pluginForLanguage(const QString& name) const {
	const auto ext = name.startsWith(".") ? name : "." + name;
	auto it = d->extToPlugin.find(ext);
	return (it != d->extToPlugin.end()) ? it->second : nullptr;
}

HighlightingPlugin* HighlightingPluginManager::pluginForContentType(const QString& name) const {
	auto it = d->contentTypeToPlugin.find(name);
	return (it != d->contentTypeToPlugin.end()) ? it->second : nullptr;
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
