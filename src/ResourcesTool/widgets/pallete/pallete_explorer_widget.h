#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "ResourcesTool/widgets/pallete/pallete_view.h"
#include "Resources/resources.h"
#include <QWidget>
#include <QScrollArea>
#include <vector>

class PalleteExplorerWidget : public BaseTabWidget {
  Q_OBJECT

public:
	PalleteExplorerWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* parent = nullptr);
	virtual ~PalleteExplorerWidget() = default;

  PaletteViewe* viewer() const;
  void setPalette(const std::vector<QRgb>& colors);

private:
	QScrollArea* _scrollArea;
	PaletteViewe* _paletteViewer;
};
