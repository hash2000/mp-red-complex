#pragma once
#include <QString>
#include <memory>

struct LanguageInfo;
class FormatterPlugin;
class Resources;

class FormatterPluginManager {
public:
	FormatterPluginManager(Resources* resources);
	~FormatterPluginManager();

	void registerPlugin(std::unique_ptr<FormatterPlugin> plugin);
	FormatterPlugin* formatter(const LanguageInfo& lang) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
