#pragma once
#include "Launcher/mdi_child_window.h"
#include <memory>

class MapEditorWindow : public MdiChildWindow {
  Q_OBJECT

public:
  explicit MapEditorWindow(const QString& id, QWidget* parent = nullptr);
  ~MapEditorWindow() override;

  QString windowType() const override { return "map-editor"; }
  QString windowTitle() const override { return "Редактор карты"; }

  bool handleCommand(const QString& commandName,
		const QStringList& args,
		CommandContext* context) override;

private:
  class Private;
  std::unique_ptr<Private> d;
};
