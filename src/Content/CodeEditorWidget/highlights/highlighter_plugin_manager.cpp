#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Content/CodeEditorWidget/highlights/theme_highlighter_plugin.h"
#include "Libs/Resources/resources.h"

#include <QDebug>
#include <QFileInfo>
#include <map>

namespace {
/// Каталог плагинов подсветки внутри активной темы.
QString highlightersDirectory(Resources* resources) {
	if (!resources) {
		return { };
	}

	const auto themeDir = resources->Directories.get("path.theme");
	if (!themeDir) {
		return { };
	}

	return themeDir->filePath("highlighters");
}
}

class HighlightingPluginManager::Private {
public:
	Private(HighlightingPluginManager* parent) : q(parent) {}
	HighlightingPluginManager* q;

	std::vector<std::unique_ptr<HighlightingPlugin>> plugins;
	std::map<QString, HighlightingPlugin*> extToPlugin;
	std::map<QString, HighlightingPlugin*> contentTypeToPlugin;
};

HighlightingPluginManager::HighlightingPluginManager(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	const auto directory = highlightersDirectory(resources);
	if (directory.isEmpty()) {
		qWarning() << "HighlightingPluginManager: theme directory is not configured";
		return;
	}

	auto plugins = ThemeHighlighterPlugin::loadDirectory(directory);
	for (auto& plugin : plugins) {
		registerPlugin(std::move(plugin));
	}

	qDebug() << "HighlightingPluginManager: loaded" << d->plugins.size()
		<< "highlighters from" << directory;
}

HighlightingPluginManager::~HighlightingPluginManager() = default;

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
	auto it = d->extToPlugin.find(ext.toLower());
	return (it != d->extToPlugin.end()) ? it->second : nullptr;
}

HighlightingPlugin* HighlightingPluginManager::pluginForContentType(const QString& name) const {
	auto it = d->contentTypeToPlugin.find(name.toLower());
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
