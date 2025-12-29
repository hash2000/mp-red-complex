#pragma once
#include <QPlainTextEdit>
#include <QByteArray>

class HexDumpWidgetSimple: public QPlainTextEdit {
Q_OBJECT;
public:
	HexDumpWidgetSimple(QWidget* parent = nullptr);

  void setByteArray(const QByteArray& data);
  QByteArray byteArray() const;
	void clear();

private:
	QString formatHexDump(const QByteArray& data);

private:
	QByteArray _data;
};
