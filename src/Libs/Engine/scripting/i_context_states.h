#pragma once
#include <QObject>
#include <QVariant>

class CommandContext;

class IContextStates {
public:
	virtual QVariant update(const CommandContext* context) = 0;
};
