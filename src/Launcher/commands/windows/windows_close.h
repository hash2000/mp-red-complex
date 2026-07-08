#pragma once
#include <QString>

class CommandContext;
class Controllers;

namespace WindowsNs {
bool handleClose(const QString& id,
	CommandContext* context,
	Controllers* controllers);
}
