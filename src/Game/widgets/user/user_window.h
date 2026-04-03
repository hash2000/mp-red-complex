#pragma once
#include "Game/mdi_child_window.h"
#include <QObject>
#include <memory>

class UsersService;
class UserWidget;
class TexturesService;

class UserWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit UserWindow(UsersService* usersService, TexturesService* texturesService, const QString& id, QWidget* parent = nullptr);
	~UserWindow() override;

	QString windowType() const override { return "user-profile"; }
	QString windowTitle() const override { return "Профиль пользователя"; }
	QSize windowDefaultSizes() const override { return QSize(400, 600); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private slots:
	void onEquipmentRequested(const QUuid& characterId);
	void onInventoryRequested(const QUuid& characterId);
	void onUserLoggedOut();

private:
	class Private;
	std::unique_ptr<Private> d;
};
