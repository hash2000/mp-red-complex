#pragma once
#include "Libs/Engine/mdi_child_window.h"
#include <QObject>
#include <memory>

class LoginWindow : public MdiChildWindow {
	Q_OBJECT
public:
	explicit LoginWindow(const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent = nullptr);
	~LoginWindow() override;

	QString windowType() const override { return "login"; }

	QString windowTitle() const override { return "Вход в систему"; }

	QSize windowDefaultSizes() const override { return QSize(400, 350); }

	bool NeedCentering() const override { return true; }

	bool NeedFixedSize() const override { return true; }

	bool handleCommand(const std::shared_ptr<Instruction> cmd) override;

private slots:
	void onLoginSuccess();
	void onRegisterSuccess();

private:
	class Private;
	std::unique_ptr<Private> d;
};
