#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Launcher/widgets/hex_control_panel.h"
#include "Game/data_format/int/data_reader.h"
#include <QLabel>
#include <QSpinBox>
#include <QWidget>
#include <QLineEdit>

StreamWidgetSelector::StreamWidgetSelector(std::weak_ptr<Resources> resources,
		QStackedWidget *centerStack,
		QVBoxLayout *actionsLayout)
: _resources(resources)
, _centerStack(centerStack)
, _actionsLayout(actionsLayout) {
}

void StreamWidgetSelector::buildStackedView() {
  _views.empty = new QWidget;
	_views.hex = new HexDumpWidget;

	_centerStack->addWidget(_views.empty);
	_centerStack->addWidget(_views.hex);

	_centerStack->setCurrentWidget(_views.empty);


  connect(_views.hex, &HexDumpWidget::selectionChanged,
          this, &StreamWidgetSelector::onHexSelectionChanged);
}

void StreamWidgetSelector::setSelection(const QStandardItem *item) {
  if (!item) {
		_selection.type = AssetsViewItemType::Undefined;
		return;
	}

	const auto index = item->index();
	_selection.container = index.data(static_cast<int>(AssetsViewItemRole::ContainerName)).toString();
	_selection.path = index.data(static_cast<int>(AssetsViewItemRole::FullPath)).toString();
	_selection.suffix = index.data(static_cast<int>(AssetsViewItemRole::Suffix)).toString();
	_selection.type = index.data(static_cast<int>(AssetsViewItemRole::Type)).value<AssetsViewItemType>();
}

bool StreamWidgetSelector::isSelected() const {
  return _selection.type != AssetsViewItemType::Undefined;
}

std::unique_ptr<QMenu> StreamWidgetSelector::buildContextMenu(QWidget *parent) const {
	auto menu = std::make_unique<QMenu>(parent);
	if (_selection.type == AssetsViewItemType::File) {
		menu
			->addAction("Hex View")
			->setObjectName("action_hex_view");
	}

	return menu;
}

void StreamWidgetSelector::displayModel(Resources& resources) {
	const auto stream = getStream(resources);
	if (!stream) {
		applyEmptyView();
		return;
	}

	auto widget = buildWidget(_selection.suffix, *stream.value());
	if (!widget) {
		return;
	}

	_centerStack->setCurrentWidget(widget);
}

void StreamWidgetSelector::displayHexView(const QByteArray& data) {
	auto resources = _resources.lock();
	if (!resources) {
		return;
	}

	auto stream = getStream(*resources);
	if (!stream) {
		return;
	}

	prepareWidget(_views.hex);

	_views.hex->setData(data);
	_centerStack->setCurrentWidget(_views.hex);

	_panels.hex = new HexControlPanel(_views.hex, stream.value());
	_actionsLayout->addWidget(_panels.hex);
	_actionsLayout->addStretch();
}

QWidget *StreamWidgetSelector::buildWidget(const QString &suffix, DataStream &stream) {

	auto block = stream.makeBlockAsStream();
	if (suffix == "int") {
		Format::Int::Programmability result;
		Format::Int::DataReader reader(*block);
		reader.read(result);
	}

	prepareWidget(_views.empty);
  return _views.empty;
}

std::optional<std::shared_ptr<DataStream>> StreamWidgetSelector::getStream(Resources& resources) const {
	if (_selection.type != AssetsViewItemType::File) {
		return std::nullopt;
	}

	return resources.getStream(
		_selection.container,
		_selection.path);
}

void StreamWidgetSelector::prepareWidget(QWidget *current) {
  QLayoutItem *item;
  while ((item = _actionsLayout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->setParent(nullptr);
      delete item->widget();
    }
    delete item;
  }

	if (_views.hex != current) {
		_panels.hex = nullptr;
		_views.hex->clear();
	}
}

void StreamWidgetSelector::applyEmptyView() {
	prepareWidget(_views.empty);
	_centerStack->setCurrentWidget(_views.empty);
}

void StreamWidgetSelector::onHexSelectionChanged(qint64 offset, qint64 length, const QByteArray& selected) {
	// if (_panels.hex) {
	// 	_panels.hex->updateFromSelection();
	// }
}
