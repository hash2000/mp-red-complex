#pragma once
#include <QString>
#include <memory>

class Instruction;
class CommandContext;
class Controllers;

namespace WindowsNs {
bool handleInvoke(const std::shared_ptr<Instruction> instruction, CommandContext* context, Controllers* controllers);
}
