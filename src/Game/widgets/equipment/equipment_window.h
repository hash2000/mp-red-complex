#pragma once
#include "Game/mdi_child_window.h"

class EquipmentWidget;
class InventoryService;

class EquipmentWindow : public MdiChildWindow {
	Q_OBJECT

public:
	EquipmentWindow(InventoryService* incentoryService, QWidget* parent = nullptr);
	~EquipmentWindow() override;

	QString windowType() const override { return "equipment"; }

	QString windowTitle() const override { return "Character equipment"; }

	QSize windowDefaultSizes() const override { return QSize(380, 500); }

	EquipmentWidget* widget() const;

	bool handleCommand(const QString& commandName,
		const QStringList& args,
		CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
