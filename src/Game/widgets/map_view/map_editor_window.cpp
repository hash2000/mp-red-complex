#include "Game/widgets/map_view/map_editor_window.h"
#include "Game/widgets/map_view/map_editor_widget.h"
#include "Game/services/time_service/time_service.h"
#include "ApplicationLayer/maps/map_service.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include <QVBoxLayout>
#include <memory>

class MapEditorWindow::Private {
public:
    Private(MapEditorWindow* parent) : q(parent) {}
    MapEditorWindow* q;

    MapService* mapService = nullptr;
    TilesService* tilesService = nullptr;
    TimeService* timeService = nullptr;
    MapEditorWidget* editorWidget = nullptr;
};

MapEditorWindow::MapEditorWindow(
    MapService* mapService,
    TilesService* tilesService,
    TimeService* timeService,
    const QString& id,
    QWidget* parent)
    : MdiChildWindow(id, parent)
    , d(std::make_unique<Private>(this)) {
    d->mapService = mapService;
    d->tilesService = tilesService;
    d->timeService = timeService;

    // Создаём виджет редактора
    d->editorWidget = new MapEditorWidget(mapService, tilesService, this);
    setWidget(d->editorWidget);

    // Подписываемся на тики для обновления
    if (d->timeService) {
        connect(d->timeService, &TimeService::tick,
                d->editorWidget, QOverload<>::of(&MapEditorWidget::update));
    }
}

MapEditorWindow::~MapEditorWindow() = default;

MapEditorWidget* MapEditorWindow::widget() const {
    return d->editorWidget;
}

bool MapEditorWindow::handleCommand(const QString& commandName,
                                     const QStringList& args,
                                     CommandContext* context) {
    // TODO: Обработка команд для редактора карты
    Q_UNUSED(commandName);
    Q_UNUSED(args);
    Q_UNUSED(context);
		return true;
}
