#pragma once
#include <QPlainTextEdit>
#include <QByteArray>

class HexDumpWidget: public QPlainTextEdit {
Q_OBJECT;
public:
	HexDumpWidget(QWidget* parent = nullptr);

  void setByteArray(const QByteArray& data);

  QByteArray byteArray() const;

	void clear();

private:
	QString formatHexDump(const QByteArray& data);

private:
	QByteArray _data;
};
