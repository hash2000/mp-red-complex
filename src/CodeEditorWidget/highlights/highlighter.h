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
	explicit Highlighter(QTextDocument* parent = nullptr);
	~Highlighter() override;

	void addEmbeddedCodeRule(const QString& name, const QString& startPattern, const QString& endPattern, const QString& languageId, const QTextCharFormat& format, int priority = 0);
	void addRule(const QString& name, const QString& pattern, const QTextCharFormat& format, int priority = 0, HighlighterRuleType type = HighlighterRuleType::Global);
	void updateRulePattern(const QString& name, const QString& pattern);
	QString getRulePattern(const QString& name) const;

	void deleteEmbeddedCodeRule(const QString& name);
	void deleteRule(const QString& name);

	void clearEmbeddedRules();
	void clearRules();

	void lockRefreshView(bool lock = true);

protected:
	void highlightBlock(const QString& text) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
