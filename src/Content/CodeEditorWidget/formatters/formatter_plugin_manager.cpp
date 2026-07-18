#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include "Content/CodeEditorWidget/formatters/plugins/external_cli_formatter.h"
#include "Content/CodeEditorWidget/language_info.h"
#include "Libs/Resources/resources.h"

#include <QDir>

#include <vector>

class FormatterPluginManager::Private {
public:
	Private(FormatterPluginManager* parent) : q(parent) {}
	FormatterPluginManager* q;

	std::vector<std::unique_ptr<FormatterPlugin>> plugins;
	std::map<QString, FormatterPlugin*> typesToPlugin;
};

FormatterPluginManager::FormatterPluginManager(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	const auto executablePath = resources->Variables.get("Resources.Tools.Formatter.Executable", "").toString();
	const auto formattersPath = QDir(resources->Variables.get("Resources.Tools.Formatters.Path", "").toString());

	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/css", executablePath, QMap<QString, QString>
		{
			{ "--parser", "css" },
			{	"--stdin-filepath", formattersPath.filePath("formatters/css/unsaved.css") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/html", executablePath, QMap<QString, QString>
		{
			{ "--parser", "html" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/html/unsaved.html") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("application/json", executablePath, QMap<QString, QString>
		{
			{ "--parser", "json" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/json/unsaved.json") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("application/javascript", executablePath, QMap<QString, QString>
		{
			{ "--parser", "js" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/js/unsaved.js") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/markdown", executablePath, QMap<QString, QString>
		{
			{ "--parser", "md" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/markdown/unsaved.md") },
		}));
}

FormatterPluginManager::~FormatterPluginManager() = default;

void FormatterPluginManager::registerPlugin(std::unique_ptr<FormatterPlugin> plugin) {
	d->typesToPlugin[plugin->mimeType().toLower()] = plugin.get();
	d->plugins.push_back(std::move(plugin));
}

FormatterPlugin* FormatterPluginManager::formatter(const LanguageInfo& lang) const {
	auto it = d->typesToPlugin.find(lang.mimeType);
	return it != d->typesToPlugin.end() ? it->second : nullptr;
}
