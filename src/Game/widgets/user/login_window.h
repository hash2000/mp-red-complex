#pragma once
#include "Game/mdi_child_window.h"
#include <QObject>
#include <memory>

class UsersService;
class LoginWidget;

class LoginWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit LoginWindow(UsersService* usersService, const QString& id, QWidget* parent = nullptr);
	~LoginWindow() override;

	QString windowType() const override { return "login"; }

	QString windowTitle() const override { return "Вход в систему"; }

	QSize windowDefaultSizes() const override { return QSize(400, 350); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private slots:
	void onLoginSuccess();
	void onRegisterSuccess();

private:
	class Private;
	std::unique_ptr<Private> d;
};
