#include "ResourcesTool/widgets/pallete/pallete_view.h"

PaletteViewe::PaletteViewe(QWidget *parent)
  : QWidget(parent)
	, _squareSize(24)
	, _spacing(2)
{
}

void PaletteViewe::setPalette(const std::vector<QRgb> &colors) {
  _colors = colors;
  updateGeometry();
  update();
}

[[nodiscard]] int PaletteViewe::squareSize() const {
	return _squareSize;
}

void PaletteViewe::setSquareSize(int size) {
  if (size > 0 && size != _squareSize) {
    _squareSize = size;
    updateGeometry();
    update();
  }
}

QSize PaletteViewe::sizeHint() const {
  if (_colors.empty()) {
    return {100, 50};
	}

  int cols = calculateColumns(width());
  int rows = (_colors.size() + cols - 1) / cols;
  return {cols * (_squareSize + _spacing) + _spacing,
          rows * (_squareSize + _spacing) + _spacing};
}

QSize PaletteViewe::minimumSizeHint() const {
  return {_squareSize + 2 * _spacing, _squareSize + 2 * _spacing};
}

void PaletteViewe::resizeEvent(QResizeEvent *) {
	updateGeometry();
}

void PaletteViewe::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, false); // для пиксель-перфекта

  const int cols = calculateColumns(width());
  const int cellSize = _squareSize + _spacing;

  for (int i = 0; i < _colors.size(); ++i) {
    int row = i / cols;
    int col = i % cols;

    QRect rect(col * cellSize + _spacing, row * cellSize + _spacing,
               _squareSize, _squareSize);

    p.fillRect(rect, QColor::fromRgba(_colors[i]));
    p.setPen(Qt::black);
    p.drawRect(rect.adjusted(0, 0, -1, -1)); // тонкая рамка
  }
}

void PaletteViewe::mousePressEvent(QMouseEvent *event) {
  if (event->button() != Qt::LeftButton)
    return;

  const int cols = calculateColumns(width());
  const int cellSize = _squareSize + _spacing;

  // Переводим позицию в индекс
  int col = (event->pos().x() - _spacing) / cellSize;
  int row = (event->pos().y() - _spacing) / cellSize;
  int index = row * cols + col;

  if (index >= 0 && index < _colors.size()) {
    QColor color = QColor::fromRgba(_colors[index]);
    QString hex = color.name(QColor::HexArgb).mid(1); // #AARRGGBB → RRGGBB
    if (hex.length() == 8)
      hex = hex.mid(2); // убираем AA, если есть
    QApplication::clipboard()->setText("#" + hex.toUpper());
    Q_EMIT colorSelected(color, index);
  }
}

int PaletteViewe::calculateColumns(int widgetWidth) const {
  if (widgetWidth <= 0)
    return 1;
  int cellSize = _squareSize + _spacing;
  int cols = (widgetWidth - _spacing) / cellSize;
  return std::max(1, cols);
}
