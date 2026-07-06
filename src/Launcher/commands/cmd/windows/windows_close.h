#pragma once
#include <QString>

class CommandContext;

namespace WindowsNs {
bool handleClose(const QString& id, CommandContext* context);
}
