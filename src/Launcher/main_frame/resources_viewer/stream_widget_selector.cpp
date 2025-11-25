#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
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
		emit beforeStreamSelection("", std::nullopt);
		return;
	}

	emit beforeStreamSelection(_selection.suffix, stream);
}

// void StreamWidgetSelector::buildWidget(const QString &suffix, DataStream &stream) {

// 	auto block = stream.makeBlockAsStream();
// 	if (suffix == "int") {
// 		prepareWidget(nullptr);
// 		auto result = std::make_unique<Format::Int::Programmability>();
// 		Format::Int::DataReader reader(*block);
// 		reader.read(*result);
// 		auto panel = new ProcedureExplorerWidget(std::move(result));
// 		_actionsLayout->addWidget(panel);
// 		_actionsLayout->addStretch();
// 		return;
// 	}
// 	else if (suffix == "txt") {
// 		prepareWidget(_views.text);
// 		QString result;
// 		Format::Txt::DataReader reader( *block);
// 		reader.read(result);
// 		_views.text->setPlainText(result);
// 		_centerStack->setCurrentWidget(_views.text);
// 		return;
// 	}

// 	prepareWidget(_views.empty);
// 	_centerStack->setCurrentWidget(_views.empty);
// }

std::optional<std::shared_ptr<DataStream>> StreamWidgetSelector::getStream(Resources& resources) const {
	if (_selection.type != AssetsViewItemType::File) {
		return std::nullopt;
	}

	return resources.getStream(
		_selection.container,
		_selection.path);
}

// void StreamWidgetSelector::prepareWidget(QWidget *current) {
//   QLayoutItem *item;
//   while ((item = _actionsLayout->takeAt(0)) != nullptr) {
//     if (item->widget()) {
//       item->widget()->setParent(nullptr);
//       delete item->widget();
//     }
//     delete item;
//   }

// 	if (_views.hex != current) {
// 		_views.hex->clear();
// 	}

// 	if (_views.text != current) {
// 		_views.text->clear();
// 	}
// }

// void StreamWidgetSelector::applyEmptyView() {
// 	prepareWidget(_views.empty);
// 	_centerStack->setCurrentWidget(_views.empty);
// }

void StreamWidgetSelector::onHexSelectionChanged(qint64 offset, qint64 length, const QByteArray& selected) {
	// if (_panels.hex) {
	// 	_panels.hex->updateFromSelection();
	// }
}
