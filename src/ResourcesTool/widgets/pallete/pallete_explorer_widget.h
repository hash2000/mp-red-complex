#pragma once
#include "ResourcesTool/widgets/pallete/pallete_view.h"
#include <QWidget>
#include <QScrollArea>
#include <vector>

class PalleteExplorerWidget : public QWidget {
  Q_OBJECT

public:
	PalleteExplorerWidget(QWidget* parent = nullptr);
	virtual ~PalleteExplorerWidget() = default;

  PaletteViewe* viewer() const;
  void setPalette(const std::vector<QRgb>& colors);

private:
	QScrollArea* _scrollArea;
	PaletteViewe* _paletteViewer;
};
