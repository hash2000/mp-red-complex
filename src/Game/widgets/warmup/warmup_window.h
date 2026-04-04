#pragma once
#include "Game/mdi_child_window.h"

class WarmupWidget;

class WarmupWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit WarmupWindow(const QString& id, QWidget* parent = nullptr);
	~WarmupWindow() override;

	QString windowType() const override { return "warmup"; }
	QString windowTitle() const override { return "OpenGL Warmup"; }
	QSize windowDefaultSizes() const override { return QSize(1, 1); }

	bool handleCommand(const QString& commandName,
		const QStringList& args,
		CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
