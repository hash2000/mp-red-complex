#pragma once
#include <QString>
#include <memory>

struct LanguageInfo;
class FormatterPlugin;

class FormatterPluginManager {
public:
	FormatterPluginManager();
	~FormatterPluginManager();

	void loadFormatters(const QString& formattersDir);
	void registerPlugin(std::unique_ptr<FormatterPlugin> plugin);
	FormatterPlugin* formatter(const LanguageInfo& lang) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
