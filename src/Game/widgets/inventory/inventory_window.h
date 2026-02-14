#pragma once
#include "Game/mdi_child_window.h"
#include <QObject>

class InventoryWidget;
class InventoriesService;
class InventoryItem;

class InventoryWindow : public MdiChildWindow {
	Q_OBJECT

public:
	InventoryWindow(InventoriesService* service, const QString& id, QWidget* parent = nullptr);
	~InventoryWindow() override;

	QString windowType() const override { return "inventory"; }

	QString windowTitle() const override;

	QSize windowDefaultSizes() const override { return QSize(460, 500); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

protected:
	void closeEvent(QCloseEvent* closeEvent) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
