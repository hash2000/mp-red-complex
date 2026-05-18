#pragma once
#include <QSyntaxHighlighter>

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit Highlighter(QTextDocument* parent = nullptr);
	~Highlighter() override;

	void addRule(const QString& name, const QString& pattern, const QTextCharFormat& format);

	void deleteRule(const QString& name);

	void clearRules();

protected:
	void highlightBlock(const QString& text) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
