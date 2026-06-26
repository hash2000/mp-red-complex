#pragma once

#include <QString>
#include <QList>
#include <QVariant>
#include <QVector>

class ConsoleTable {
public:
	// Конструктор с колонками
	explicit ConsoleTable(const QStringList& columnNames = { });

	// Добавление колонки
	void addColumn(const QString& name);

	// Добавление строки (значения по порядку колонок)
	void addRow(const QList<QVariant>& values);

	// Добавление строки с именованными значениями (гибкий вариант)
	void addRow(const QHash<QString, QVariant>& namedValues);

	// Установка выравнивания для колонки (по умолчанию - по левому краю)
	enum Alignment {
		Left, Right, Center
	};
	void setColumnAlignment(int columnIndex, Alignment alignment);
	void setColumnAlignment(const QString& columnName, Alignment alignment);

	// Генерация HTML-таблицы
	QString toHtml() const;

	// Вспомогательные методы
	bool isEmpty() const {
		return _rows.isEmpty();
	}
	int rowCount() const {
		return _rows.size();
	}
	int columnCount() const {
		return _columns.size();
	}

private:
	struct Column {
		QString name;
		Alignment alignment = Left;
	};

	QStringList _columns;
	QVector<Column> _columnData;
	QList<QList<QVariant>> _rows;

	// Внутренние вспомогательные методы
	QString escapeHtml(const QString& text) const;
	QString alignmentToCss(Alignment align) const;
};
