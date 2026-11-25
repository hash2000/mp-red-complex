#pragma once
#include <QFont>
#include <QString>
#include <QTextLayout>
#include <QTextLine>

/// Раскладка одной логической строки текста: переносы, табы и отображение
/// позиции символа в экранные координаты.
///
/// Класс не хранит состояние выделения и не рисует — его можно использовать
/// и для просмотра, и для будущего редактирования/многокурсорности.
class TextLayout {
public:
	static constexpr int kDefaultTabWidth = 2;

	TextLayout(const QString& text, const QFont& font, bool wrapped,
		int width, int tabWidth = kDefaultTabWidth,
		const QVector<QTextLayout::FormatRange>& formats = {});

	int lineCount() const;
	QTextLine lineAt(int visualRow) const;

	/// Экранная строка, содержащая абсолютную колонку.
	int visualRowForColumn(int column) const;
	/// Геометрия экранной строки с колонкой (y и высота относительно строки).
	bool rowForColumn(int column, int& visualRow, int& y, int& height) const;
	/// Экранная строка под координатой y; top/y — относительно текста строки.
	bool rowAtY(int y, int& visualRow, int& top, int& height) const;

	qreal xForColumn(int column) const;
	int columnAt(int visualRow, qreal x) const;
	qreal naturalWidth(int visualRow) const;

	/// Форматы подсветки в координатах строки. Вызывать после создания
	/// раскладки и до отрисовки.
	void setFormats(const QVector<QTextLayout::FormatRange>& formats);

private:
	QTextLayout layout;
};
