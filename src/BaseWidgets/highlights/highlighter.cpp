#include "BaseWidgets/highlights/highlighter.h"
#include <QRegularExpression>
#include <map>

namespace {
	struct HighlightingRule
	{
		QRegularExpression pattern;
		QTextCharFormat format;
	};
}

class Highlighter::Private {
public:
	Private(Highlighter* parent) : q(parent) {}
	Highlighter* q;

	std::map<QString, HighlightingRule> rules;
};

Highlighter::Highlighter(QTextDocument* parent)
	: QSyntaxHighlighter(parent)
	, d(std::make_unique<Private>(this)) {
}

Highlighter::~Highlighter() = default;

void Highlighter::addRule(const QString& name, const QString& pattern, const QTextCharFormat& format) {
	HighlightingRule rule;
	rule.pattern = QRegularExpression(pattern);
	rule.format = format;

	d->rules.emplace(name, rule);

	rehighlight();
}

void Highlighter::deleteRule(const QString& name) {
	auto it = d->rules.find(name);
	if (it == d->rules.end()) {
		return;
	}

	d->rules.erase(it);

	rehighlight();
}

void Highlighter::clearRules() {
	d->rules.clear();
	rehighlight();
}

void Highlighter::highlightBlock(const QString& text) {
	for (const auto& [name, rule] : d->rules)
	{
		QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
		while (it.hasNext())
		{
			QRegularExpressionMatch match = it.next();
			setFormat(match.capturedStart(), match.capturedLength(), rule.format);
		}
	}
}
