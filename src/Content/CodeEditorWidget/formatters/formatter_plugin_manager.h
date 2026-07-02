#pragma once
#include <QString>
#include <memory>


class FormatterPluginManager {
public:
	FormatterPluginManager();
	~FormatterPluginManager();

	void loadFormatters(const QString& formattersDir);

private:
	class Private;
	std::unique_ptr<Private> d;
};
