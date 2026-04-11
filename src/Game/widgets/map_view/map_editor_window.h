#pragma once
#include "Game/mdi_child_window.h"
#include <memory>

class MapEditorWidget;
class MapService;
class TilesService;
class TimeService;

class MapEditorWindow : public MdiChildWindow {
    Q_OBJECT

public:
    explicit MapEditorWindow(
        MapService* mapService,
        TilesService* tilesService,
        TimeService* timeService,
        const QString& id,
        QWidget* parent = nullptr);
    ~MapEditorWindow() override;

    QString windowType() const override { return "map-editor"; }
    QString windowTitle() const override { return "Редактор карты"; }

    MapEditorWidget* widget() const;

    bool handleCommand(const QString& commandName,
                       const QStringList& args,
                       CommandContext* context) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};
