#pragma once
#include "Game/widgets/map_view/map_view_base.h"
#include "Game/services/time_service/time_events.h"

class ShadersService;

class MapWidget : public MapViewBase {
	Q_OBJECT
public:
	explicit MapWidget(ShadersService* shadersService, TilesSelectorService* tilesSelectorService, QWidget* parent = nullptr);
	~MapWidget() override;

public slots:
	void onTick(const TickEvent& event);
};
