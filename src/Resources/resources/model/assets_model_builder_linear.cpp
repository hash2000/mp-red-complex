#include "Resources/resources/model/assets_model_builder_linear.h"
#include "Resources/resources/model/assets_model_builder_recursive.h"
#include "Resources/resources/model/assets_model.h"
#include "Base/scoped_timer.h"

AssetsModelBuilderLinear::AssetsModelBuilderLinear(QStandardItem* parent,
	const std::shared_ptr<Resources>& resources)
	: _parent(parent)
	, _resources(resources) {
}

void AssetsModelBuilderLinear::build() {
	if (_parent->rowCount() == 1) {
		const auto item = _parent->child(0);
		const auto dummyValue = item->data(static_cast<int>(AssetsViewItemRole::DummyItem));
		if (dummyValue != true) {
			return;
		}

		_parent->removeRow(0);
	}
	else {
		return;
	}

	const auto type = _parent->data(static_cast<int>(AssetsViewItemRole::Type)).value<AssetsViewItemType>();

	if (type == AssetsViewItemType::Container) {
		const auto name = _parent->data(static_cast<int>(AssetsViewItemRole::ContainerName)).toString();
		for (const auto& res : _resources->items()) {
			if (res.name() != name) {
				continue;
			}

			switch(res.type()) {
			case ContainerType::Repository_v1:
				AssetsModelBuilderRecursive(_parent, res)
					.build();
				return;
			case ContainerType::Directory:
				AssetsModelBuilderRecursive(_parent, res)
					.build();
				break;
			}
		}
	}

}

