#include "ResourcesTool/main_frame/resources_viewer/stream_widget_selector.h"
#include <QLabel>
#include <QSpinBox>
#include <QWidget>
#include <QLineEdit>

StreamWidgetSelector::StreamWidgetSelector() {
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

QString StreamWidgetSelector::getContainerName() const {
	return _selection.container;
}

QString StreamWidgetSelector::getContainerPath() const {
	return _selection.path;
}

QVariantMap StreamWidgetSelector::getSelection() const {
	return QVariantMap({
		{ "container.name", _selection.container },
		{ "container.path", _selection.path },
		{ "suffix", _selection.suffix },
		{ "asset.type", (unsigned char)_selection.type },
		});
}

void StreamWidgetSelector::execute() {
	emit beforeStreamSelection(getSelection());
}

std::optional<std::shared_ptr<DataStream>> StreamWidgetSelector::getStream(Resources& resources) const {
	if (_selection.type != AssetsViewItemType::File) {
		return std::nullopt;
	}

	return resources.getStream(
		_selection.container,
		_selection.path);
}
