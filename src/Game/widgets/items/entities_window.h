#pragma once
#include "DataLayer/items/item.h"
#include "Game/mdi_child_window.h"
#include <QObject>

class ItemsService;

class EntitiesWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit EntitiesWindow(ItemsService* service, const QString& id, QWidget* parent = nullptr);
	~EntitiesWindow() override;

	QString windowType() const override { return "items"; }

	QString windowTitle() const { return "Библиотека предметов"; }

	QSize windowDefaultSizes() const override { return QSize(460, 500); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private slots:
	void onItemCreateRequested(const QString& itemId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
