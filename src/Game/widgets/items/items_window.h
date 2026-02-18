#pragma once
#include "DataLayer/items/item.h"
#include "Game/mdi_child_window.h"
#include <QObject>

class ItemsService;

class ItemsWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit ItemsWindow(ItemsService* service, const QString& id, QWidget* parent = nullptr);
	~ItemsWindow() override;

	QString windowType() const override { return "items"; }

	QString windowTitle() const { return "Items"; }

	QSize windowDefaultSizes() const override { return QSize(460, 500); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
