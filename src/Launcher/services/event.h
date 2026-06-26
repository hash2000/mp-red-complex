#pragma once
#include <QDateTime>
#include <QString>

struct Event {
	QString eventType;
	QDateTime timestamp;

	explicit Event(const QString& type)
		: eventType(type)
		, timestamp(QDateTime::currentDateTime()) {
	}
};
