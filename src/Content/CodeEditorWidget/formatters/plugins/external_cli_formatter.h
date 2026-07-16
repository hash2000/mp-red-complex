#pragma once
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include <QMap>
#include <memory>

class ExternalCliFormatterPlugin : public FormatterPlugin {
	Q_OBJECT
public:
	ExternalCliFormatterPlugin(
		const QString& mimeType,
		const QString& formatterProgram,
		const QMap<QString, QString> &arguments);
	~ExternalCliFormatterPlugin() override;

	QString mimeType() const override;
	void formatAsync(const QString& text) override;
	void stop() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
