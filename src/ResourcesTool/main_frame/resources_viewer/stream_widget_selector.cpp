#include "ResourcesTool/main_frame/resources_viewer/stream_widget_selector.h"
#include <QLabel>
#include <QSpinBox>
#include <QWidget>
#include <QLineEdit>

StreamWidgetSelector::StreamWidgetSelector(std::weak_ptr<Resources> resources)
: _resources(resources) {
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

AssetsViewItemType StreamWidgetSelector::getType() const {
	return _selection.type;
}

QString StreamWidgetSelector::getSuffix() const {
	return _selection.suffix;
}

void StreamWidgetSelector::displayModel(Resources& resources) {
	const auto stream = getStream(resources);
	if (!stream) {
		emit beforeStreamSelection("", std::nullopt);
		return;
	}

	emit beforeStreamSelection(_selection.suffix, stream);
}

std::optional<std::shared_ptr<DataStream>> StreamWidgetSelector::getStream(Resources& resources) const {
	if (_selection.type != AssetsViewItemType::File) {
		return std::nullopt;
	}

	return resources.getStream(
		_selection.container,
		_selection.path);
}

void StreamWidgetSelector::onHexSelectionChanged(qint64 offset, qint64 length, const QByteArray& selected) {
	// if (_panels.hex) {
	// 	_panels.hex->updateFromSelection();
	// }
}
