#pragma once

#include <QPalette>
#include <QString>

namespace PaletteParser {
QPalette parseFromFile(const QString& filePath);
}
