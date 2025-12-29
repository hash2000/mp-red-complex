#pragma once

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>
#include <vector>

class PaletteViewe : public QWidget {
  Q_OBJECT

public:
  PaletteViewe(QWidget *parent = nullptr);
  virtual ~PaletteViewe() = default;

  void setPalette(const std::vector<QRgb> &colors);

  [[nodiscard]] int squareSize() const;
  void setSquareSize(int size);

protected:
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
  void resizeEvent(QResizeEvent *) override;
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *event) override;

Q_SIGNALS:
  void colorSelected(const QColor &color, int index);

private:
  int calculateColumns(int widgetWidth) const;

private:
  std::vector<QRgb> _colors;
  int _squareSize;
  int _spacing;
};
