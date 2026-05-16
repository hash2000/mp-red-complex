#pragma once
#include "Game/mdi_child_window.h"
#include <memory>

class MaterialWidget;
class QTabWidget;
class QSplitter;
class QTreeView;
class QTableView;
class QWidget;

class MaterialsWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit MaterialsWindow(const QString& id, QWidget* parent = nullptr);
	~MaterialsWindow() override;

	QString windowType() const override { return "materials"; }
	QString windowTitle() const override { return "Materials"; }
	QSize windowDefaultSizes() const override { return QSize(1024, 768); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private:
	void setupUi();

private:
	class Private;
	std::unique_ptr<Private> d;
};
