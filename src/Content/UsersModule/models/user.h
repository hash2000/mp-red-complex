#pragma once
#include <QUuid>
#include <QString>
#include <QDateTime>
#include <QHash>
#include <QPixmap>

/// Данные пользователя
class UserData {
public:
	QString loginHash;
	QString passwordHash;
	QString displayName;
	QDateTime createdAt;

	QString iconPath;
	QPixmap icon;
};
