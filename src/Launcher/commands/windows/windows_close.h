#pragma once
#include <QString>

class CommandContext;
class Controllers;

namespace WindowsNs {
bool handleClose(const QString& id, std::shared_ptr<CommandContext> context, Controllers* controllers);
}
