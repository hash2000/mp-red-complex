#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"

class FormatterPluginManager::Private {
public:
	Private(FormatterPluginManager* parent) : q(parent) {}
	FormatterPluginManager* q;

	//std::vector<std::unique_ptr<HighlightingPlugin>> plugins;
	//std::map<QString, HighlightingPlugin*> extToPlugin;
};

FormatterPluginManager::FormatterPluginManager()
	: d(std::make_unique<Private>(this)) {}

FormatterPluginManager::~FormatterPluginManager() = default;

void FormatterPluginManager::loadFormatters(const QString& formattersDir) {

}
