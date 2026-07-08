#include "Content/ConsoleModule/models/instruction.h"
#include <QJsonDocument>
#include <QDomDocument>

bool Instruction::hasParameter(const QString& name) const {
	return parameters.contains(name);
}

QVariant Instruction::parameter(const QString& name) const {
	return parameters.value(name);
}

// Convenience methods
QString Instruction::text(const QString& name, const QString& defaultValue) const {
	return parameters.value(name).toString();
}

QJsonDocument Instruction::json(const QString& name) const {
	return parameters.value(name).toJsonDocument();
}

QDomDocument Instruction::xml(const QString& name) const {
	return parameters.value(name).value<QDomDocument>();
}

// Для INI можно использовать QMap<QString, QVariant>
QMap<QString, QVariant> Instruction::ini(const QString& name) const {
	return parameters.value(name).toMap();
}
