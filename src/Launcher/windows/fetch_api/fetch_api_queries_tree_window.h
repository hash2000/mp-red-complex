#pragma once
#include "Libs/Engine/mdi_child_window.h"
#include <QWidget>
#include <memory>

class FetchApiQueriesTreeWindow : public MdiChildWindow {
	Q_OBJECT
public:
	FetchApiQueriesTreeWindow(const QString& id, QWidget* parent = nullptr);
	~FetchApiQueriesTreeWindow() override;

	QString windowType() const override {
		return "fetch-api-queries-tree";
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
