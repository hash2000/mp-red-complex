#include "ResourcesTool/widgets/pallete/pallete_explorer_widget.h"
#include "DataFormat/data_format/pal/data_reader.h"
#include <QVBoxLayout>

PalleteExplorerWidget::PalleteExplorerWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
	QWidget *parent)
: BaseTabWidget(resources, params, parent) {

	auto block = currentStream();
	auto result = std::make_unique<Proto::Pallete>();
	DataFormat::Pal::DataReader reader(*block);
	reader.read(*result);


  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  _scrollArea = new QScrollArea;
  _scrollArea->setWidgetResizable(true);
  _scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  _paletteViewer = new PaletteViewe;
	_paletteViewer->setPalette(result->items);
  _scrollArea->setWidget(_paletteViewer);

  layout->addWidget(_scrollArea);
}

PaletteViewe* PalleteExplorerWidget::viewer() const {
	return _paletteViewer;
}

void PalleteExplorerWidget::setPalette(const std::vector<QRgb>& colors) {
  _paletteViewer->setPalette(colors);
}
