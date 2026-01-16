#include "BaseWidgets/tabs/base_tab_widget.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QJsonDocument>

BaseTabWidget::BaseTabWidget(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget* parent)
: QWidget(parent)
, _resources(resources)
, _params(params) {
	setAcceptDrops(true);
}

std::shared_ptr<DataStream> BaseTabWidget::currentStream() const {
	const auto containerName = _params.value("container.name").toString();
	const auto containerPath = _params.value("container.path").toString();

	auto blockOpt = _resources->getStream(containerName, containerPath);
	auto block = blockOpt.value()->makeBlockAsStream();
	return block;
}

void BaseTabWidget::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasFormat("application/x-container")) {
		event->acceptProposedAction();
	}
}

void BaseTabWidget::dropEvent(QDropEvent* event) {
	auto data = event->mimeData()->data("application/x-container");
	QJsonDocument doc = QJsonDocument::fromJson(data);
	QVariantMap map = doc.toVariant().toMap();

	emit requestContainer(map);

	event->acceptProposedAction();
}
