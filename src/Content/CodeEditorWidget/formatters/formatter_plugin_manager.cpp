#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include "Content/CodeEditorWidget/formatters/plugins/external_cli_formatter.h"
#include "Content/CodeEditorWidget/language_info.h"

#include <vector>

class FormatterPluginManager::Private {
public:
	Private(FormatterPluginManager* parent) : q(parent) {}
	FormatterPluginManager* q;

	std::vector<std::unique_ptr<FormatterPlugin>> plugins;
	std::map<QString, FormatterPlugin*> typesToPlugin;
};

FormatterPluginManager::FormatterPluginManager()
	: d(std::make_unique<Private>(this)) {
}

FormatterPluginManager::~FormatterPluginManager() = default;

void FormatterPluginManager::loadFormatters(const QString& formattersDir) {

}

void FormatterPluginManager::registerPlugin(std::unique_ptr<FormatterPlugin> plugin) {
	const auto& mimeTypes = plugin->mimeTypes();
	for (const auto type : mimeTypes) {
		d->typesToPlugin[type.toLower()] = plugin.get();
	}

	d->plugins.push_back(std::move(plugin));
}

FormatterPlugin* FormatterPluginManager::formatter(const LanguageInfo& lang) const {
	auto it = d->typesToPlugin.find(lang.mimeType);
	return it != d->typesToPlugin.end() ? it->second : nullptr;
}
