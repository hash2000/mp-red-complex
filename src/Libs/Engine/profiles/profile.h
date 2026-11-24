#pragma once
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"

#include <QDir>
#include <QString>

#include <memory>

/// Профиль пользователя: каталог настроек в рабочей конфигурации
/// (<path.profile>/profiles/<имя>) и права профиля.
///
/// Настройки лежат в appearance.yaml и перекрывают значения по умолчанию из
/// bootstrap.yaml. Пустой профиль при первом открытии инициализируется этими
/// значениями, дальше файл можно менять вручную или из программы, не трогая
/// конфигурацию приложения.
class Profile {
public:
	Profile(QString name, const QDir& directory);
	~Profile();

	Profile(const Profile&) = delete;
	Profile& operator=(const Profile&) = delete;

	QString name() const;
	QDir directory() const;

	/// Путь к appearance.yaml — настройкам профиля.
	QString settingsPath() const;

	/// Профиль неавторизованного пользователя.
	bool isDefault() const;

	/// Профилю по умолчанию доступны только открытые (незашифрованные) базы.
	bool allowsEncryptedStores() const;

	/// Читает appearance.yaml. Отсутствующий или пустой файл создаётся из
	/// defaults; недостающие ключи берутся из defaults.
	bool load(const BootstrapConfig::Appearance& defaults);

	/// Сохраняет настройки профиля (пригодится настройкам в программе).
	bool save(const BootstrapConfig::Appearance& appearance);

	const BootstrapConfig::Appearance& appearance() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
