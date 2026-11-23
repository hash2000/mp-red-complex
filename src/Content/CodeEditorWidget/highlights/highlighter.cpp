#include "Content/CodeEditorWidget/highlights/highlighter.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>

namespace {
/// Строки длиннее этого предела не подсвечиваются.
///
/// Одна строка в десятки килобайт — это, как правило, минифицированная
/// разметка. Регулярные выражения на такой строке растут квадратично
/// (жадные `[^...]+` с возвратом), из-за чего подсветка одной строки
/// занимает секунды и вешает редактор при каждой правке. Форматы для
/// таких строк не нужны: читать их построчно всё равно невозможно.
constexpr int kMaxHighlightedBlockLength = 2048;
}

class Highlighter::Private {
public:
	Private(Highlighter* parent) : q(parent) {}
	Highlighter* q;

	HighlightingPluginManager* pluginManager = nullptr;
	HighlightingPlugin* plugin = nullptr;
	HighlightingPlugin* embeddedSyntaxPlugin = nullptr;

	bool lazy = false;
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
	d->embeddedSyntaxPlugin = nullptr;

	// После смены языка старые форматы не должны оставаться: в ленивом
	// режиме rehighlight() покрасит только видимые строки.
	clearFormats();
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
	d->embeddedSyntaxPlugin = nullptr;
	clearFormats();
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

void Highlighter::refreshVisibleBlocks() {
	QTextDocument* doc = document();
	if (!doc || !d->plugin) {
		return;
	}

	const int last = qMin(d->visibleLast, doc->blockCount() - 1);
	if (d->visibleFirst == d->refreshedFirst && last == d->refreshedLast) {
		return;
	}

	// Состояние вложенных блоков за пределами видимой области неизвестно:
	// начинаем с чистого состояния на первой видимой строке.
	d->embeddedSyntaxPlugin = nullptr;

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
}

void Highlighter::highlightBlock(const QString& text) {
	if (!d->plugin) {
		return;
	}

	// Длинные строки пропускаем до регулярных выражений: иначе правка
	// одной такой строки пересчитывает её целиком и подвешивает ввод.
	// Форматы на длинной строке ещё и заставляют документ заново
	// раскладывать её целиком при каждом обновлении подсветки.
	if (text.length() > kMaxHighlightedBlockLength) {
		setCurrentBlockState(0);
		return;
	}

	// В ленивом режиме невидимые строки остаются без формата до прокрутки.
	if (d->lazy && !d->isVisible(currentBlock().blockNumber())) {
		setCurrentBlockState(0);
		return;
	}

	if (d->plugin->hasEmbeddedRules()) {
		bool handled = false;
		d->plugin->visitEmbeddedMatches(text,
			[this, &handled](qsizetype start, qsizetype length,
				const QTextCharFormat& format, int,
				HighlightingPlugin::HighlightingMatch::Type type,
				const QString& language) {
				// Как и раньше, для строки учитывается только первое совпадение.
				if (handled) {
					return;
				}
				handled = true;
				setFormat(static_cast<int>(start), static_cast<int>(length), format);

				const bool isStart = type == HighlightingPlugin::HighlightingMatch::Type::Start;
				if (isStart && !d->embeddedSyntaxPlugin) {
					const QString embeddedLanguage = language.isEmpty()
						? QStringLiteral("bash") : language;
					d->embeddedSyntaxPlugin =
						d->pluginManager->pluginForLanguage(embeddedLanguage);
				}
				else if (!isStart) {
					d->embeddedSyntaxPlugin = nullptr;
				}
			});

		if (handled) {
			return;
		}
	}

	HighlightingPlugin* plugin = d->embeddedSyntaxPlugin
		? d->embeddedSyntaxPlugin : d->plugin;
	plugin->visitMatches(text,
		[this](qsizetype start, qsizetype length, const QTextCharFormat& format, int) {
			setFormat(static_cast<int>(start), static_cast<int>(length), format);
		});
}
