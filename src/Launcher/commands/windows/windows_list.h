#pragma once
#include <QString>
#include <memory>

class CommandContext;
class Controllers;

namespace WindowsNs {
bool handleList(std::shared_ptr<CommandContext> context, Controllers* controllers);
}
