#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Game/data_format/int/data_reader.h"

StreamWidgetSelector::StreamWidgetSelector(std::weak_ptr<Resources> resources)
: _resources(resources) {
}

void StreamWidgetSelector::buildStackedView(QStackedWidget *view) {
  _views.empty = new QWidget;
	_views.hex = new HexDumpWidget;

	view->addWidget(_views.empty);
	view->addWidget(_views.hex);

	view->setCurrentWidget(_views.empty);
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

void StreamWidgetSelector::displayModel(QStackedWidget *view) {
	const auto stream = getStream();
	if (!stream) {
		applyEmptyView(view);
		return;
	}

	auto widget = buildWidget(_selection.suffix, stream.value());
	if (!widget) {
		return;
	}

	clenViewsData(widget);
	view->setCurrentWidget(widget);
}

void StreamWidgetSelector::displayHexView(QStackedWidget *view) {
	const auto streamOpt = getStream();
	if (!streamOpt) {
		applyEmptyView(view);
		return;
	}

	DataStream& stream = streamOpt.value();

	auto block = stream.readBlockAsQByteArray();
	_views.hex->setByteArray(block);
	view->setCurrentWidget(_views.hex);
}

QWidget *StreamWidgetSelector::buildWidget(const QString &suffix, DataStream &stream) const {
	auto block = stream.makeBlockAsStream();
	if (suffix == "int") {
		Format::Int::Programmability result;
		Format::Int::DataReader reader(*block);
		reader.read(result);
	}

  return _views.empty;
}

std::optional<std::reference_wrapper<DataStream>> StreamWidgetSelector::getStream()
{
	const auto resources = _resources.lock();
	if (_selection.type != AssetsViewItemType::File) {
		return std::nullopt;
	}

	if (!resources) {
		return std::nullopt;
	}

	return resources->getStream(_selection.container, _selection.path);
}

void StreamWidgetSelector::clenViewsData(QWidget *current) {
	if (_views.hex != current) {
		_views.hex->clear();
	}
}

void StreamWidgetSelector::applyEmptyView(QStackedWidget *view)
{
	clenViewsData();
	view->setCurrentWidget(_views.empty);
}
