#pragma once
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include <memory>

class ExternalCliFormatterPlugin : public FormatterPlugin {
	Q_OBJECT
public:
	ExternalCliFormatterPlugin(const QString& formatterProgramm, const QStringList& arguments);
	~ExternalCliFormatterPlugin() override;

	QStringList mimeTypes() const override;
	void formatAsync(const QString& text) override;
	void stop() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
