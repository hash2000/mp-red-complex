#include "Libs/BaseWidgets/editor/highlights/highlighter.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>

namespace {
/// Длинная строка не подсвечивается вовсе: регулярные выражения на ней
/// растут квадратично (жадные `[^...]+` с возвратом), и подсветка одной
/// строки занимает секунды. Граница — Highlighter::MaxHighlightedBlockLength.
}

class Highlighter::Private {
public:
	Private(Highlighter* parent) : q(parent) {}
	Highlighter* q;

	HighlightingPluginManager* pluginManager = nullptr;
	HighlightingPlugin* plugin = nullptr;

	/// Состояние вложенного блока: внутри ли мы и на каком языке содержимое.
	/// Пустой язык — блок без вложенной подсветки: строки внутри него
	/// подсвечиваются правилами базового плагина.
	bool insideEmbedded = false;
	QString embeddedLanguage;

	bool lazy = false;
	/// Подсветка включена: false — форматы сняты и правила не выполняются.
	bool enabled = true;
	int visibleFirst = 0;
	int visibleLast = 0;
	/// Диапазон, для которого форматы уже посчитаны: повторный вызов
	/// refreshVisibleBlocks() без прокрутки не должен ничего пересчитывать.
	int refreshedFirst = -1;
	int refreshedLast = -1;

	bool isVisible(int blockNumber) const {
		return blockNumber >= visibleFirst && blockNumber <= visibleLast;
	}
};

Highlighter::Highlighter(HighlightingPluginManager* pluginManager, QTextDocument* parent)
	: QSyntaxHighlighter(parent)
	, d(std::make_unique<Private>(this)) {
	d->pluginManager = pluginManager;
}

Highlighter::~Highlighter() = default;

const HighlightingPlugin* Highlighter::plugin() const {
	return d->plugin;
}

bool Highlighter::setLanguage(const QString& language) {
	HighlightingPlugin* plugin = d->pluginManager ? d->pluginManager->pluginForLanguage(language)
		: nullptr;
	if (plugin == d->plugin) {
		return plugin != nullptr;
	}

	d->plugin = plugin;
	d->insideEmbedded = false;
	d->embeddedLanguage.clear();

	// После смены языка старые форматы не должны оставаться: в ленивом
	// режиме rehighlight() покрасит только видимые строки.
	clearFormats();
	if (!d->enabled) {
		return plugin != nullptr;
	}
	if (d->lazy) {
		refreshVisibleBlocks();
	}
	else {
		rehighlight();
	}

	return plugin != nullptr;
}

bool Highlighter::setContentType(const QString& type) {
	HighlightingPlugin* plugin = d->pluginManager
		? d->pluginManager->pluginForContentType(type) : nullptr;
	if (plugin == d->plugin) {
		return plugin != nullptr;
	}

	d->plugin = plugin;
	d->insideEmbedded = false;
	d->embeddedLanguage.clear();
	clearFormats();
	if (!d->enabled) {
		return plugin != nullptr;
	}
	if (d->lazy) {
		refreshVisibleBlocks();
	}
	else {
		rehighlight();
	}

	return plugin != nullptr;
}

bool Highlighter::isLazyMode() const {
	return d->lazy;
}

void Highlighter::setLazyMode(bool lazy) {
	// Только переключает режим: вызывающая сторона сама решает, когда
	// пересчитывать подсветку (rehighlight() или refreshVisibleBlocks()).
	d->lazy = lazy;
}

void Highlighter::setVisibleBlockRange(int first, int last) {
	if (first < 0) {
		first = 0;
	}
	if (last < first) {
		last = first;
	}

	d->visibleFirst = first;
	d->visibleLast = last;
}

bool Highlighter::isEnabled() const {
	return d->enabled;
}

void Highlighter::setEnabled(bool enabled) {
	if (d->enabled == enabled) {
		return;
	}

	d->enabled = enabled;
	if (!enabled) {
		// Убираем и старые цвета: выключенная подсветка не должна
		// оставлять разукрашенный текст.
		clearFormats();
		return;
	}

	if (!d->plugin) {
		return;
	}

	if (d->lazy) {
		refreshVisibleBlocks();
	}
	else {
		rehighlight();
	}
}

void Highlighter::refreshVisibleBlocks() {
	QTextDocument* doc = document();
	if (!doc || !d->plugin || !d->enabled) {
		return;
	}

	const int last = qMin(d->visibleLast, doc->blockCount() - 1);
	if (d->visibleFirst == d->refreshedFirst && last == d->refreshedLast) {
		return;
	}

	// Состояние вложенных блоков за пределами видимой области неизвестно:
	// начинаем с чистого состояния на первой видимой строке.
	d->insideEmbedded = false;
	d->embeddedLanguage.clear();

	QTextBlock block = doc->findBlockByNumber(d->visibleFirst);
	for (int number = d->visibleFirst; block.isValid() && number <= last;
		number++, block = block.next()) {
		rehighlightBlock(block);
	}

	d->refreshedFirst = d->visibleFirst;
	d->refreshedLast = last;
}

void Highlighter::clearFormats() {
	QTextDocument* doc = document();
	if (!doc) {
		return;
	}

	QTextCursor cursor(doc);
	cursor.beginEditBlock();
	for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
		if (QTextLayout* layout = block.layout()) {
			layout->clearFormats();
		}
	}
	cursor.endEditBlock();

	// Форматы живут в раскладке, а не в тексте: без этого документ
	// не узнал бы, что видимые строки нужно перерисовать.
	doc->markContentsDirty(0, doc->characterCount());

	d->refreshedFirst = -1;
	d->refreshedLast = -1;
	d->insideEmbedded = false;
	d->embeddedLanguage.clear();
}

void Highlighter::highlightBlock(const QString& text) {
	if (!d->plugin) {
		return;
	}

	// Документ с слишком длинной строкой не подсвечивается целиком:
	// вызывающая сторона снимает запрет, когда строка станет короче.
	if (!d->enabled) {
		setCurrentBlockState(0);
		return;
	}

	// Защита на случай, если длинную строку ещё не заметили снаружи:
	// регулярные выражения на ней растут квадратично и подвешивают ввод.
	if (text.length() > MaxHighlightedBlockLength) {
		setCurrentBlockState(0);
		return;
	}

	// В ленивом режиме невидимые строки остаются без формата до прокрутки.
	if (d->lazy && !d->isVisible(currentBlock().blockNumber())) {
		setCurrentBlockState(0);
		return;
	}

	struct EmbeddedMatch {
		int start = 0;
		int length = 0;
		QTextCharFormat format;
		bool startOfBlock = false;
		QString language;
	};

	QVector<EmbeddedMatch> matches;
	if (d->plugin->hasEmbeddedRules()) {
		d->plugin->visitEmbeddedMatches(text,
			[&matches](qsizetype start, qsizetype length,
				const QTextCharFormat& format, int,
				HighlightingPlugin::HighlightingMatch::Type type,
				const QString& language) {
				matches.append({ static_cast<int>(start),
					static_cast<int>(length), format,
					type == HighlightingPlugin::HighlightingMatch::Type::Start,
					language });
			});
	}

	// Строку внутри вложенного блока подсвечиваем языком этого блока; если
	// язык не задан (например, тег HTML), остаются правила базового плагина.
	HighlightingPlugin* contentPlugin = d->plugin;
	if (d->insideEmbedded && !d->embeddedLanguage.isEmpty() && d->pluginManager) {
		if (HighlightingPlugin* embedded =
			d->pluginManager->pluginForLanguage(d->embeddedLanguage)) {
			contentPlugin = embedded;
		}
	}
	contentPlugin->visitMatches(text,
		[this](qsizetype start, qsizetype length, const QTextCharFormat& format, int) {
			setFormat(static_cast<int>(start), static_cast<int>(length), format);
		});

	// Форматы start/end накладываются последними: они важнее подсветки
	// содержимого. Обрабатываются все совпадения строки, а не только первое:
	// так подсвечиваются и однострочные блоки, и закрывающий тег.
	for (const EmbeddedMatch& match : matches) {
		if (match.length > 0) {
			setFormat(match.start, match.length, match.format);
		}
		if (match.startOfBlock) {
			d->insideEmbedded = true;
			d->embeddedLanguage = match.language;
		}
		else {
			d->insideEmbedded = false;
			d->embeddedLanguage.clear();
		}
	}
}
