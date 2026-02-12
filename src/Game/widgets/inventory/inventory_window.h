#pragma once
#include "Game/mdi_child_window.h"

class InventoryWidget;
class InventoriesService;
class InventoryItem;

class InventoryWindow : public MdiChildWindow {
	Q_OBJECT

public:
	InventoryWindow(InventoriesService* inventoriesService, const QString& id, QWidget* parent = nullptr);
	~InventoryWindow() override;

	QString windowType() const override { return "inventory"; }

	QString windowTitle() const override { return "Character inventory"; }

	QSize windowDefaultSizes() const override { return QSize(380, 500); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
