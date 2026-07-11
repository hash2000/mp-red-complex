#pragma once

#include <QString>
#include <QMap>
#include <QVariant>
#include <memory>

class QJsonDocument;
class QDomDocument;

class Instruction {
public:
	QString command;
	QMap<QString, QVariant> parameters;
	QMap<QString, QString> parametersMimeTypes;

	QString mimetype(const QString& name) const;

	bool hasParameter(const QString& name) const;

	QVariant parameter(const QString& name) const;

	// Convenience methods
	QString text(const QString& name, const QString& defaultValue = { }) const;

	QJsonDocument json(const QString& name) const;

	QDomDocument xml(const QString& name) const;

	// Для INI можно использовать QMap<QString, QVariant>
	QMap<QString, QVariant> ini(const QString& name) const;
};
