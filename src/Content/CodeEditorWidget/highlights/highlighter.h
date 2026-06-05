#pragma once
#include <QSyntaxHighlighter>

class HighlightingPluginManager;

enum class HighlighterRuleType {
	Embedded,
	Global
};

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit Highlighter(HighlightingPluginManager* pluginManager, QTextDocument* parent = nullptr);
	~Highlighter() override;

	void setLanguage(const QString& language);

protected:
	void highlightBlock(const QString& text) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
