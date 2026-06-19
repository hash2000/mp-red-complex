#include "Launcher/commands/command_console/console_table.h"

#include <QTextDocument>

ConsoleTable::ConsoleTable(const QStringList& columnNames) {
	for (const QString& name : columnNames) {
		addColumn(name);
	}
}

void ConsoleTable::addColumn(const QString& name) {
	_columns.append(name);
	_columnData.append({ name, Left });
}

void ConsoleTable::addRow(const QList<QVariant>& values) {
	if (values.size() > _columns.size()) {
		qWarning() << "Row has more values than columns:"
			<< values.size() << "vs" << _columns.size();
	}

	// Дополняем пустыми значениями, если не хватает
	QList<QVariant> row = values;
	while (row.size() < _columns.size()) {
		row.append(QString());
	}

	_rows.append(row);
}

void ConsoleTable::addRow(const QHash<QString, QVariant>& namedValues) {
	QList<QVariant> row;
	row.reserve(_columns.size());

	for (const QString& columnName : _columns) {
		if (namedValues.contains(columnName)) {
			row.append(namedValues.value(columnName));
		}
		else {
			row.append(QString()); // пустое значение по умолчанию
		}
	}

	_rows.append(row);
}

void ConsoleTable::setColumnAlignment(int columnIndex, Alignment alignment) {
	if (columnIndex >= 0 && columnIndex < _columnData.size()) {
		_columnData[columnIndex].alignment = alignment;
	}
}

void ConsoleTable::setColumnAlignment(const QString& columnName, Alignment alignment) {
	for (int i = 0; i < _columnData.size(); ++i) {
		if (_columnData[i].name == columnName) {
			_columnData[i].alignment = alignment;
			return;
		}
	}
}

QString ConsoleTable::escapeHtml(const QString& text) const {
	return text.toHtmlEscaped()
		.replace("\r\n", "\n")
		.replace('\n', "<br>")
		.replace('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
}

QString ConsoleTable::alignmentToCss(Alignment align) const {
	switch (align) {
	case Left: return "text-align: left;";
	case Right: return "text-align: right;";
	case Center: return "text-align: center;";
	default: return "text-align: left;";
	}
}

QString ConsoleTable::toHtml() const {
	if (_columns.isEmpty()) {
		return QString();
	}

	QString html = "<table class=\"console-table\">\n";

	// Заголовки таблицы
	html += "  <thead>\n    <tr>";
	for (const Column& col : _columnData) {
		QString cssAlign = alignmentToCss(col.alignment);
		html += QString("\n      <th style=\"%1\">%2</th>")
			.arg(cssAlign)
			.arg(escapeHtml(col.name));
	}
	html += "\n    </tr>\n  </thead>\n";

	// Тело таблицы
	if (!_rows.isEmpty()) {
		html += "  <tbody>\n";
		for (const QList<QVariant>& row : _rows) {
			html += "    <tr>";
			for (int i = 0; i < row.size() && i < _columnData.size(); ++i) {
				QString cellContent;
				const QVariant& value = row[i];

				if (value.isNull()) {
					cellContent = "";
				}
				else if (value.type() == QVariant::String) {
					cellContent = value.toString();
				}
				else if (value.canConvert<QString>()) {
					cellContent = value.toString();
				}
				else {
					cellContent = "[unsupported type]";
				}

				QString cssAlign = alignmentToCss(_columnData[i].alignment);
				html += QString("\n      <td style=\"%1\">%2</td>")
					.arg(cssAlign)
					.arg(escapeHtml(cellContent));
			}
			html += "\n    </tr>\n";
		}
		html += "  </tbody>\n";
	}

	html += "</table>";
	return html;
}
