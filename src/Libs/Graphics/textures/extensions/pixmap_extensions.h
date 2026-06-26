#pragma once
#include <QPixmap>

namespace Extensions::Pixmaps {
QPixmap scale(const QPixmap& src, int width = -1, int height = -1);
}
