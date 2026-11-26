#include "Libs/BaseWidgets/editor/navigation/text_navigation.h"

#include <QtGlobal>

#include <vector>

namespace {

/// Печатный символ в смысле стратегии: буква или цифра. Пунктуация и
/// пробельные символы разделяют слова.
bool isWordCharacter(QChar character) {
	return character.isLetterOrNumber();
}

} // namespace

QString WordTextNavigationStrategy::id() const {
	return QStringLiteral("words");
}

QString WordTextNavigationStrategy::title() const {
	return QStringLiteral("По словам");
}

int WordTextNavigationStrategy::columnAfterMove(const QString& line, int column,
	int direction) const {
	column = qBound(0, column, line.size());
	if (direction == 0) {
		return column;
	}

	if (direction < 0) {
		if (column <= 0) {
			return 0;
		}

		const bool printable = isWordCharacter(line.at(column - 1));
		int position = column - 1;
		while (position > 0 && isWordCharacter(line.at(position - 1)) == printable) {
			position--;
		}
		return position;
	}

	if (column >= line.size()) {
		return line.size();
	}

	const bool printable = isWordCharacter(line.at(column));
	int position = column;
	while (position < line.size()
		&& isWordCharacter(line.at(position)) == printable) {
		position++;
	}
	return position;
}

const WordTextNavigationStrategy& WordTextNavigationStrategy::instance() {
	static const WordTextNavigationStrategy strategy;
	return strategy;
}

class TextNavigationStrategyRegistry::Private {
public:
	std::vector<std::unique_ptr<ITextNavigationStrategy>> strategies;
};

TextNavigationStrategyRegistry::TextNavigationStrategyRegistry()
	: d(std::make_unique<Private>()) {
	registerStrategy(std::make_unique<WordTextNavigationStrategy>());
}

TextNavigationStrategyRegistry::~TextNavigationStrategyRegistry() = default;

void TextNavigationStrategyRegistry::registerStrategy(
	std::unique_ptr<ITextNavigationStrategy> strategy) {
	if (!strategy) {
		return;
	}

	const QString id = strategy->id();
	for (auto& existing : d->strategies) {
		if (existing->id() == id) {
			existing = std::move(strategy);
			return;
		}
	}
	d->strategies.push_back(std::move(strategy));
}

QList<ITextNavigationStrategy*> TextNavigationStrategyRegistry::strategies() const {
	QList<ITextNavigationStrategy*> result;
	result.reserve(static_cast<int>(d->strategies.size()));
	for (const auto& strategy : d->strategies) {
		result.append(strategy.get());
	}
	return result;
}

ITextNavigationStrategy* TextNavigationStrategyRegistry::strategy(
	const QString& id) const {
	for (const auto& strategy : d->strategies) {
		if (strategy->id() == id) {
			return strategy.get();
		}
	}
	return nullptr;
}

QStringList TextNavigationStrategyRegistry::ids() const {
	QStringList result;
	for (const auto& strategy : d->strategies) {
		result << strategy->id();
	}
	return result;
}

QString TextNavigationStrategyRegistry::defaultId() const {
	return WordTextNavigationStrategy::instance().id();
}
