#pragma once
#include <QAbstractScrollArea>
#include <QByteArray>
#include <QMap>
#include <QTimer>

class HexDumpWidget : public QAbstractScrollArea {
  Q_OBJECT

public:
  explicit HexDumpWidget(QWidget *parent = nullptr);

  void setData(const QByteArray &data);
  void clear();
	qsizetype getDataSize() const;

  // API для внешнего управления
  void selectRange(qint64 offset, qint64 length);
  void scrollToByte(qint64 offset);
  QByteArray selectedData() const;
  QPair<qint64, qint64> selectedRange() const;

  // Настройки
  void setHighlightDuplicates(bool enabled);
  void setBytesPerRow(int count);
  void setFontSize(int pt);

signals:
  void selectionChanged(qint64 offset, qint64 length, const QByteArray& selected);

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private:
  struct LayoutMetrics {
    int rowHeight = 0;
    int charWidth = 0;
    int offsetWidth = 0;
    int hexStartX = 0;
    int asciiStartX = 0;
    int bytesPerRow = 16;
  };

  void updateMetrics();
  void updateDuplicateMap();
  qint64 posToByte(const QPoint &pos) const;
  void emitSelectionChange();

  QByteArray _data;
  LayoutMetrics _metrics;
  QFont _font;

  // Выделение
  qint64 _selStart = -1;
  qint64 _selEnd = -1;
  bool _dragging = false;

  // Дубликаты
  bool _highlightDuplicates = false;
  QMap<quint8, int> _byteFrequency; // byte → count

  // Таймер для отложенного обновления дубликатов (на больших данных)
  QTimer _duplicateUpdateTimer;
};
