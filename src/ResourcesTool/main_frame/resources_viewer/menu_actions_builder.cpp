#include "ResourcesTool/main_frame/resources_viewer/menu_actions_builder.h"

MenuActionsBuilder::MenuActionsBuilder(QWidget *parent, std::shared_ptr<StreamWidgetSelector> selector)
: _parent(parent)
, _selector(selector) {
}

std::unique_ptr<QMenu> MenuActionsBuilder::Build() {
	const auto type = _selector->getType();
	auto menu = std::make_unique<QMenu>(_parent);
	if (type == AssetsViewItemType::File) {
		menu
			->addAction("Hex View")
			->setObjectName("action_hex_view");

		menu
			->addAction("Text View")
			->setObjectName("action_text_view");
	} else if (type == AssetsViewItemType::Container) {
		menu
			->addAction("Extract ...")
			->setObjectName("action_extract_container_to");
	}

	return menu;
}
