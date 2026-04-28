#include "Game/widgets/map_view/map_widget.h"
#include "Game/services/time_service/time_events.h"

MapWidget::MapWidget(TilesSelectorService* tilesSelectorService, QWidget* parent)
    : MapViewBase(parent) {
    setTilesService(tilesSelectorService);
}

MapWidget::~MapWidget() = default;

void MapWidget::onTick(const TickEvent& event) {
    Q_UNUSED(event);
    update();
}
