#include "ResourcesTool/widgets/pallete/pallete_explorer_widget.h"
#include <QVBoxLayout>

PalleteExplorerWidget::PalleteExplorerWidget(QWidget *parent)
	: QWidget(parent) {

  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  _scrollArea = new QScrollArea;
  _scrollArea->setWidgetResizable(true);
  _scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  _paletteViewer = new PaletteViewe;
  _scrollArea->setWidget(_paletteViewer);

  layout->addWidget(_scrollArea);
}

PaletteViewe* PalleteExplorerWidget::viewer() const {
	return _paletteViewer;
}

void PalleteExplorerWidget::setPalette(const std::vector<QRgb>& colors) {
  _paletteViewer->setPalette(colors);
}
