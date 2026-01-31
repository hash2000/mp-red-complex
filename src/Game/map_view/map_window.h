#pragma once
#include "Game/mdi_child_window.h"

class MapWidget;

class MapWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit MapWindow(QWidget* parent = nullptr);
	~MapWindow() override;

	QString windowType() const override { return "map"; }

	// Доступ к внутреннему виджету для специфичных операций
	MapWidget* mapWidget() const;

	// Пример специфичной команды для карты
	bool handleCommand(const QString& commandName,
		const QStringList& args,
		CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
