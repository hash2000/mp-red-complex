#include "Content/MaterialsModule/widgets/material_objects/material_object_menu_actions.h"
#include "Content/MaterialsModule/widgets/material_objects/material_object_node.h"

MaterialObjectMenuActionsBuilder::MaterialObjectMenuActionsBuilder(MaterialObjectTypes type, QWidget* parent)
	: _type(type)
	, _parent(parent) {
}

std::unique_ptr<QMenu> MaterialObjectMenuActionsBuilder::buildAddMenu() {
	auto menu = std::make_unique<QMenu>(_parent);
	if (_type == MaterialObjectTypes::MaterialRoot || _type == MaterialObjectTypes::Directory) {
		menu->addAction("Material")->setObjectName("action_add_materail");
		menu->addAction("Directory")->setObjectName("action_add_directory");
	}
	else if (_type == MaterialObjectTypes::Material) {
		menu->addAction("Texture")->setObjectName("action_add_texture");
		menu->addAction("Fragment Shader")->setObjectName("action_add_frag_shader");
		menu->addAction("Vertex Shader")->setObjectName("action_add_vert_shader");
		menu->addAction("Albedo")->setObjectName("action_add_albedo");
	}

	if (_type != MaterialObjectTypes::MaterialRoot) {
		menu->addAction("Delete")->setObjectName("action_delete");
	}

	return menu;
}

QAction* MaterialObjectMenuActionsBuilder::findAction(QMenu& menu, const QString& name) {
	for (QAction* action : menu.actions()) {
		if (action->objectName() == name) {
			return action;
		}
	}
	return nullptr;
}

bool MaterialObjectMenuActionsBuilder::attachMenuAction(QMenu& menu, const QString& name, std::function<void()> callback) {
	auto action = findAction(menu, name);
	if (!action) {
		return false;
	}

	QObject::connect(action, &QAction::triggered, callback);
	return true;
}
