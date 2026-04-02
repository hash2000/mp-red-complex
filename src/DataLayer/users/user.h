#pragma once
#include <QUuid>
#include <QString>
#include <QHash>

/// Данные пользователя
struct UserData {
	QString loginHash;
	QString passwordHash;
	QString displayName;
	QHash<QString, QString> metadata;
	std::list<QUuid> characters;
	QUuid chestId;
};
