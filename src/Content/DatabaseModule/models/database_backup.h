#pragma once
#include <QDateTime>
#include <QString>
#include <QtGlobal>

/// Зарегистрированное в системе хранилище — контекст `.store.<alias>`.
///
/// Описание собирается из контекста и файловой системы: путь, шифрование и
/// настройки WAL приходят из `.store`, признаки файла — из каталога профиля.
struct DatabaseStoreInfo {
	QString alias;
	QString path;
	bool encrypted = false;
	int walCheckInterval = 0;
	int walMaxSizeMb = 0;
	bool fileExists = false;
	qint64 fileSize = 0;
	int backupCount = 0;
	QDateTime lastBackupAt;
};

/// Файл резервной копии базы данных.
struct DatabaseBackupInfo {
	QString alias;
	QString path;
	QDateTime createdAt;
	qint64 size = 0;
	/// Копия, созданная автоматически перед восстановлением.
	bool automatic = false;
};
