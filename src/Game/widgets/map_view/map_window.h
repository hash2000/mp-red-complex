#pragma once
#include "Game/mdi_child_window.h"

class MapWidget;
class WorldService;
class TimeService;

class MapWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit MapWindow(WorldService* worldService, TimeService* timeService, QWidget* parent = nullptr);
	~MapWindow() override;

	QString windowType() const override { return "map"; }

	QString windowTitle() const override { return "World Map"; }

	MapWidget* mapWidget() const;

	bool handleCommand(const QString& commandName,
		const QStringList& args,
		CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
