#pragma once

#include <QVariant>

namespace LazyTreeNodeRole {
constexpr int RawData = Qt::UserRole + 1;
constexpr int Id = Qt::UserRole + 2;
constexpr int IsDummy = Qt::UserRole + 3;
constexpr int IsTemp = Qt::UserRole + 4;
}
