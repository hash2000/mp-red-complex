#include "Game/widgets/map_view/map_widget.h"
#include "Game/services/time_service/time_events.h"

MapWidget::MapWidget(TilesService* tilesService, QWidget* parent)
    : MapViewBase(parent) {
    setTilesService(tilesService);
}

MapWidget::~MapWidget() = default;

void MapWidget::onTick(const TickEvent& event) {
    Q_UNUSED(event);
    update();
}
