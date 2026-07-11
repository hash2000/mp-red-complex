#pragma once
#include "Libs/Engine/mdi_child_window.h"
#include <QWidget>
#include <memory>

class FetchApiWindow : public MdiChildWindow {
	Q_OBJECT
public:
	FetchApiWindow(const QString& id, QWidget* parent = nullptr);
	~FetchApiWindow() override;

	QString windowType() const override {
		return "fetch-api";
	}
	QString windowTitle() const override;
	QSize windowDefaultSizes() const override {
		return QSize(800, 600);
	}
	QString help() const;

	bool handleCommand(const std::shared_ptr<Instruction> instruction, CommandContext* context) override;


private:
	class Private;
	std::unique_ptr<Private> d;
};
