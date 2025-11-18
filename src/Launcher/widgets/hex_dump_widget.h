#pragma once
#include "Launcher/widgets/hex_dump_model.h"
#include <QWidget>
#include <QByteArray>
#include <QPair>
#include <QTableView>

class HexDumpWidget : public QWidget {
  Q_OBJECT

public:
	HexDumpWidget(QWidget* parent = nullptr);

  void setByteArray(const QByteArray& data);

  QByteArray byteArray() const;

	void clear();

  // Выделяет блок [offset, offset + length), скроллит, возвращает данные
  QByteArray selectRange(qint64 offset, qint64 length);

  // Альтернатива без копирования (если данные уже в _data):
  QPair<qint64, qint64> selectedRange() const;  // возвращает {offset, length}

signals:
  void selectionChanged(qint64 offset, qint64 length);

private slots:
  void onSelectionChanged();

private:
  void scrollToByte(qint64 offset);
  void setSelectionRange(qint64 offset, qint64 length);

  QByteArray _data;
  QTableView *_tableView = nullptr;
  HexDumpModel *_model = nullptr;

  qint64 _selectionOffset = -1;
  qint64 _selectionLength = 0;
};
