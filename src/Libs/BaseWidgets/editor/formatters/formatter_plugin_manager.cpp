#include "Libs/BaseWidgets/editor/formatters/formatter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin.h"
#include "Libs/BaseWidgets/editor/formatters/plugins/external_cli_formatter.h"
#include "Libs/BaseWidgets/editor/language_info.h"
#include "Libs/Resources/resources.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

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
	const auto toolsPath = resources->Directories.get("path.tools");
	const QDir toolsDirectory = toolsPath ? toolsPath.value() : QDir();

	// По умолчанию форматтер запускается входящим в профиль node
	// со скриптом prettier; переменные ресурсов могут это переопределить.
	auto executablePath = resources->Variables.get("Resources.Tools.Formatter.Executable", "").toString();
	if (executablePath.isEmpty()) {
		executablePath = toolsDirectory.filePath("bin/node/node.exe");
	}

	auto scriptPath = resources->Variables.get("Resources.Tools.Formatter.Script", "").toString();
	if (scriptPath.isEmpty()) {
		scriptPath = toolsDirectory.filePath("bin/node/node_modules/prettier/bin/prettier.cjs");
	}

	const auto formattersVariable = resources->Variables.get("Resources.Tools.Formatters.Path", "").toString();
	const auto formattersPath = formattersVariable.isEmpty() ? toolsDirectory : QDir(formattersVariable);

	QStringList leadingArguments;
	if (QFileInfo::exists(scriptPath)) {
		leadingArguments << scriptPath;
	}

	if (!QFileInfo::exists(executablePath)) {
		qWarning() << "FormatterPluginManager: formatter executable is not found:" << executablePath;
	}

	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/css", executablePath, QMap<QString, QString>
		{
			{ "--parser", "css" },
			{	"--stdin-filepath", formattersPath.filePath("formatters/css/unsaved.css") },
		}, leadingArguments));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/html", executablePath, QMap<QString, QString>
		{
			{ "--parser", "html" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/html/unsaved.html") },
		}, leadingArguments));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("application/json", executablePath, QMap<QString, QString>
		{
			{ "--parser", "json" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/json/unsaved.json") },
		}, leadingArguments));
	// Имена парсеров соответствуют prettier 3: `js` и `md` там отсутствуют,
	// из-за чего форматирование JavaScript и Markdown молча не запускалось.
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("application/javascript", executablePath, QMap<QString, QString>
		{
			{ "--parser", "babel" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/js/unsaved.js") },
		}, leadingArguments));
	registerPlugin(std::make_unique<ExternalCliFormatterPlugin>("text/markdown", executablePath, QMap<QString, QString>
		{
			{ "--parser", "markdown" },
			{ "--stdin-filepath", formattersPath.filePath("formatters/markdown/unsaved.md") },
		}, leadingArguments));
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
