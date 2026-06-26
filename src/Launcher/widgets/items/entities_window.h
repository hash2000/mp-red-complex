#pragma once
#include "Content/InventoriesModule/models/item.h"
#include "Launcher/mdi_child_window.h"
#include <QObject>

class EntitiesWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit EntitiesWindow(const QString& id, QWidget* parent = nullptr);
	~EntitiesWindow() override;

	QString windowType() const override { return "entities"; }

	QString windowTitle() const { return "Библиотека предметов"; }

	QSize windowDefaultSizes() const override { return QSize(460, 500); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private slots:
	void onItemCreateRequested(const QString& itemId, const QString& inventoryId);
	void onInventorySelectionRequested();

private:
	class Private;
	std::unique_ptr<Private> d;
};
