#include "Game/widgets/materials/material_objects/material_object_menu_actions.h"
#include "Game/widgets/materials/material_objects/material_object_node.h"

MaterialObjectMenuActionsBuilder::MaterialObjectMenuActionsBuilder(MaterialObjectTypes type, QWidget* parent)
	: _type(type)
	, _parent(parent) {
}

std::unique_ptr<QMenu> MaterialObjectMenuActionsBuilder::BuildAddMenu() {
	auto menu = std::make_unique<QMenu>(_parent);
	if (_type == MaterialObjectTypes::MaterialRoot || _type == MaterialObjectTypes::Directory) {
		menu->addAction("Material")->setObjectName("action_add_materail");
		menu->addAction("Directory")->setObjectName("action_add_directory");
		return menu;
	}

	if (_type == MaterialObjectTypes::Material) {
		menu->addAction("Texture")->setObjectName("action_add_texture");
		menu->addAction("Fragment Shader")->setObjectName("action_add_frag_shader");
		menu->addAction("Vertex Shader")->setObjectName("action_add_vert_shader");
		menu->addAction("Vertex Shader")->setObjectName("action_add_vert_shader");
		return menu;
	}

	return menu;
}
