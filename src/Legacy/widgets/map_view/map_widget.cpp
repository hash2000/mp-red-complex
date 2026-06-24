#include "Launcher/widgets/map_view/map_widget.h"
#include "Launcher/services/time_service/time_events.h"

MapWidget::MapWidget(ShadersService* shadersService, TilesSelectorService* tilesSelectorService, QWidget* parent)
    : MapViewBase(shadersService, parent) {
}

MapWidget::~MapWidget() = default;

void MapWidget::onTick(const TickEvent& event) {
    Q_UNUSED(event);
    update();
}
