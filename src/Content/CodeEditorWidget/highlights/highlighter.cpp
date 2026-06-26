#include "Content/CodeEditorWidget/highlights/highlighter.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"
#include <QRegularExpression>
#include <map>

namespace {
	struct HighlightingRule {
		QRegularExpression pattern;
		QTextCharFormat format;
		int priority = 0; // 0 = высокий, 100 = низкий
	};

	struct HighlightingRuleEmbedded {
		QRegularExpression startPattern;
		QRegularExpression endPattern;
		QString languageId;
		QTextCharFormat format;
		int priority = 0;
	};

	struct HighlightingMatch {
		qsizetype start;
		qsizetype length;
		QTextCharFormat format;
		int priority = 0;
	};
}

class Highlighter::Private {
public:
	Private(Highlighter* parent) : q(parent) {}
	Highlighter* q;

	HighlightingPluginManager* pluginManager;
	HighlightingPlugin* plugin = nullptr;
	HighlightingPlugin* embeddedSynataxPlugin = nullptr;
};

Highlighter::Highlighter(HighlightingPluginManager* pluginManager, QTextDocument* parent)
	: QSyntaxHighlighter(parent)
	, d(std::make_unique<Private>(this)) {
	d->pluginManager = pluginManager;
}

Highlighter::~Highlighter() = default;

void Highlighter::setLanguage(const QString& language) {
	d->plugin = d->pluginManager->pluginForLanguage(language);
	rehighlight();
}

void Highlighter::highlightBlock(const QString& text) {
	if (!d->plugin) {
		return;
	}

	auto isEmbeddedLanguage = d->embeddedSynataxPlugin != nullptr;
	auto matches = d->plugin->matchesEmbeddedBlock(text);
	if (!matches.empty()) {
		const auto& match = *matches.begin();
		const auto isStart = match.type == HighlightingPlugin::HighlightingMatch::Type::Start;
		setFormat(match.start, match.length, match.format);

		if (isStart && !isEmbeddedLanguage) {
			const auto language = match.language.isEmpty() ? "bash" : match.language;
			d->embeddedSynataxPlugin = d->pluginManager->pluginForLanguage(language);
		}
		else if (!isStart) {
			d->embeddedSynataxPlugin = nullptr;
		}

		return;
	}

	auto plugin = d->embeddedSynataxPlugin ? d->embeddedSynataxPlugin : d->plugin;

	matches = plugin->matches(text);
	if (!matches.empty()) {
		for (const auto& match : matches) {
			setFormat(match.start, match.length, match.format);
		}
	}
}
