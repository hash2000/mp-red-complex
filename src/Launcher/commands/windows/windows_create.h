#pragma once
#include <QString>
#include <memory>

class CommandContext;
class Controllers;

namespace WindowsNs {
bool handleCreate(std::shared_ptr<CommandContext> context, Controllers* controllers);
}
