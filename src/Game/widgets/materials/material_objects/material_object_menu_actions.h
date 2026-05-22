#pragma once
#include <QMenu>
#include <memory>

enum class MaterialObjectTypes;

class MaterialObjectMenuActionsBuilder {
public:
	MaterialObjectMenuActionsBuilder(MaterialObjectTypes type, QWidget* parent);
	std::unique_ptr<QMenu> BuildAddMenu();

private:
	MaterialObjectTypes _type;
	QWidget* _parent;
};
