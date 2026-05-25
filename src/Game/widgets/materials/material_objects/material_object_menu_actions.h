#pragma once
#include <QMenu>
#include <QAction>
#include <memory>
#include <functional>

enum class MaterialObjectTypes;

class MaterialObjectMenuActionsBuilder {
public:
	MaterialObjectMenuActionsBuilder(MaterialObjectTypes type, QWidget* parent);

	std::unique_ptr<QMenu> buildAddMenu();

	static QAction* findAction(QMenu& menu, const QString& name);
	static bool attachMenuAction(QMenu& menu, const QString& name, std::function<void()> callback);

private:
	MaterialObjectTypes _type;
	QWidget* _parent;
};
