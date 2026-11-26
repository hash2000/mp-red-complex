#include "Libs/BaseWidgets/extensions/text_edit_extensions.h"

namespace {

template <typename TextWidget>
TextWidget* applyTabDistance(TextWidget* obj, int distance) {
	if (!obj) {
		return nullptr;
	}

	const int charWidth = qMax(1, obj->fontMetrics().horizontalAdvance(QLatin1Char('W')));
	obj->setTabStopDistance(qreal(Extensions::TextEdit::normalizedTabWidth(distance))
		* charWidth);
	return obj;
}

} // namespace

namespace Extensions::TextEdit {

int normalizedTabWidth(int spaces) {
	return qMax(1, spaces);
}

QTextEdit* setTabDistance(QTextEdit* obj, int distance) {
	return applyTabDistance(obj, distance);
}

QPlainTextEdit* setTabDistance(QPlainTextEdit* obj, int distance) {
	return applyTabDistance(obj, distance);
}

QStringList tabModeIds() {
	return { QStringLiteral("spaces"), QStringLiteral("tabs") };
}

QStringList tabModeTitles() {
	return { QStringLiteral("Пробелы"), QStringLiteral("Табы") };
}

TabMode tabModeFromString(const QString& value, TabMode fallback) {
	const QString normalized = value.trimmed().toLower();
	if (normalized == QStringLiteral("spaces")) {
		return TabMode::Spaces;
	}
	if (normalized == QStringLiteral("tabs")) {
		return TabMode::Tabs;
	}
	return fallback;
}

QString tabModeToString(TabMode mode) {
	return mode == TabMode::Tabs ? QStringLiteral("tabs") : QStringLiteral("spaces");
}

QString indentText(int spaces, TabMode mode) {
	if (mode == TabMode::Tabs) {
		return QStringLiteral("\t");
	}
	return QString(normalizedTabWidth(spaces), QLatin1Char(' '));
}

QString convertLeadingTabsToSpaces(const QString& text, int spaces) {
	const int width = normalizedTabWidth(spaces);
	QString result;
	result.reserve(text.size());

	bool atLineStart = true;
	for (const QChar character : text) {
		if (atLineStart) {
			if (character == QLatin1Char('\t')) {
				result.append(QString(width, QLatin1Char(' ')));
				continue;
			}
			if (character == QLatin1Char(' ')) {
				result.append(character);
				continue;
			}
		}

		result.append(character);
		atLineStart = character == QLatin1Char('\n');
	}

	return result;
}

QString textForSave(const QString& text, int spaces, TabMode mode) {
	return mode == TabMode::Spaces
		? convertLeadingTabsToSpaces(text, spaces) : text;
}

} // namespace Extensions::TextEdit
