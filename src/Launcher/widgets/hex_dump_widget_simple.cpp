#include "Launcher/widgets/hex_dump_widget_simple.h"
#include <QFontDatabase>
#include <QFont>

HexDumpWidgetSimple::HexDumpWidgetSimple(QWidget *parent)
: QPlainTextEdit(parent) {
  setReadOnly(true);
  setLineWrapMode(QPlainTextEdit::NoWrap);

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPointSize(10);
  setFont(font);
}

void HexDumpWidgetSimple::setByteArray(const QByteArray &data) {
	_data = data;
  setPlainText(formatHexDump(data));
}

QByteArray HexDumpWidgetSimple::byteArray() const {
  return _data;
}

void HexDumpWidgetSimple::clear() {
	if (_data.isEmpty()) {
		return;
	}

	_data.clear();
	_data.squeeze();
}

QString HexDumpWidgetSimple::formatHexDump(const QByteArray &data) {
  if (data.isEmpty()) {
		return "(empty)\n";
	}

  QStringList lines;
  constexpr int bytesPerLine = 16;

  for (int i = 0; i < data.size(); i += bytesPerLine) {
    QByteArray line = data.mid(i, bytesPerLine);
    QString hexPart, asciiPart;

    // Offset (8 hex digits)
    hexPart += QString("%1: ")
			.arg(i, 8, 16, QLatin1Char('0'));

    // Hex bytes
    for (int j = 0; j < bytesPerLine; ++j) {
        if (j < line.size()) {
          hexPart += QString("%1 ")
						.arg(static_cast<uint8_t>(line[j]), 2, 16, QLatin1Char('0'))
						.toUpper();
        } else {
          hexPart += "   ";
        }

        if (j == 7) {
					// разделитель после 8-го байта
					hexPart += " ";
				}
    }

    // ASCII part
    asciiPart = " ";
    for (char c : line) {
      if (c >= 32 && c <= 126) {
        asciiPart += QChar::fromLatin1(c);
      } else {
        asciiPart += '.';
      }
    }

    lines << (hexPart + asciiPart);
  }

  return lines.join('\n');
}
