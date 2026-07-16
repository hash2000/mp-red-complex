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

	QString prettierPath;
	std::vector<std::unique_ptr<FormatterPlugin>> plugins;
	std::map<QString, FormatterPlugin*> typesToPlugin;
};

FormatterPluginManager::FormatterPluginManager(const QString& toolsPath)
	: d(std::make_unique<Private>(this)) {
	auto toolsDir = QDir(toolsPath);
#ifdef Q_OS_WIN
	d->prettierPath = toolsDir.filePath("bin/node/prettier.cmd");
#else
	d->prettierPath = toolsDir.filePath("bin/node/prettier");
#endif

	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/css", d->prettierPath, QMap<QString, QString>
		{
			{ "--parser", "css" },
			{	"--stdin-filepath", toolsDir.filePath("formatters/css/unsaved.css") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/html", d->prettierPath, QMap<QString, QString>
		{
			{ "--parser", "html" },
			{ "--stdin-filepath", toolsDir.filePath("formatters/html/unsaved.html") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("application/json", d->prettierPath, QMap<QString, QString>
		{
			{ "--parser", "json" },
			{ "--stdin-filepath", toolsDir.filePath("formatters/json/unsaved.json") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("application/javascript", d->prettierPath, QMap<QString, QString>
		{
			{ "--parser", "js" },
			{ "--stdin-filepath", toolsDir.filePath("formatters/js/unsaved.js") },
		}));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/markdown", d->prettierPath, QMap<QString, QString>
		{
			{ "--parser", "md" },
			{ "--stdin-filepath", toolsDir.filePath("formatters/markdown/unsaved.md") },
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
