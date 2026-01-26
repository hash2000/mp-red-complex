#include "Launcher/map/player.h"

void Player::move(int dx, int dy) {
	pos += QPoint(dx, dy);
}
