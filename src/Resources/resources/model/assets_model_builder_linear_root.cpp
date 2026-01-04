#include "Resources/resources/model/assets_model_builder_linear_root.h"
#include "Resources/resources/model/assets_model.h"
#include "Base/scoped_timer.h"

AssetsModelBuilderLinearRoot::AssetsModelBuilderLinearRoot(QStandardItem* parent,
	const std::shared_ptr<Resources>& resources)
	: _parent(parent)
	, _resources(resources) {
}

void AssetsModelBuilderLinearRoot::build() {
	for (const auto& res : _resources->items()) {
		ScopedTimer watch(QString("Build files tree for resource %1").arg(res.name()));
		buildFromContainer(res);
	}
}

void AssetsModelBuilderLinearRoot::buildFromContainer(const DataStreamContainer& container) {
	const auto name = QString("%1 *")
		.arg(container.name());

	auto containerItem = new QStandardItem(name);
	containerItem->setData(container.name(), static_cast<int>(AssetsViewItemRole::ContainerName));
	containerItem->setData((unsigned char)AssetsViewItemType::Container, static_cast<int>(AssetsViewItemRole::Type));
	containerItem->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DriveHarddisk));
	_parent->appendRow(containerItem);

	auto dummyItem = new QStandardItem("dummy");
	dummyItem->setData(true, static_cast<int>(AssetsViewItemRole::DummyItem));
	containerItem->appendRow(dummyItem);
}
