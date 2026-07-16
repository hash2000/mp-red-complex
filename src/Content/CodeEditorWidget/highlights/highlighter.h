#pragma once
#include <QSyntaxHighlighter>

class HighlightingPluginManager;
class HighlightingPlugin;

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit Highlighter(HighlightingPluginManager* pluginManager, QTextDocument* parent = nullptr);
	~Highlighter() override;

	const HighlightingPlugin* plugin() const;
	bool setLanguage(const QString& language);
	bool setContentType(const QString& type);

protected:
	void highlightBlock(const QString& text) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
