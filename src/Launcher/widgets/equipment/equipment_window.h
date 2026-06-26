#pragma once
#include "Launcher/mdi_child_window.h"

class EquipmentWindow : public MdiChildWindow {
	Q_OBJECT

public:
	EquipmentWindow(const QString& id, QWidget* parent = nullptr);
	~EquipmentWindow() override;

	QString windowType() const override { return "equipment"; }

	QString windowTitle() const override { return "Character equipment"; }

	QSize windowDefaultSizes() const override { return QSize(380, 500); }

	bool NeedFixedSize() const override { return true; }

	bool handleCommand(const QString& commandName,
		const QStringList& args,
		CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
