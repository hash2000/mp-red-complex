#pragma once
#include "Libs/Resources/variables/variables_context.h"
#include "Libs/Resources/directories/directories_context.h"
#include "Libs/DataStream/data_stream/data_stream_container.h"
#include "Libs/Base/container_view.h"

#include <QDir>

#include <list>
#include <memory>
#include <optional>


class DataWriteStream;

class Resources {
public:
	Resources();
	virtual ~Resources();

public:
	bool load();
	bool loadProfile(const QString& userHash = QString());
	std::optional<QDir> createProfilePath(const QString& userHash, bool checkOnly = false);
	QString defaultUseerName() const;

	/// Текущий профиль настроек: default для неавторизованного пользователя,
	/// user hash — после входа.
	QString currentProfileName() const;
	std::optional<QDir> currentProfilePath() const;

	/// Профилю default доступны только открытые (незашифрованные) базы.
	/// До выбора профиля ограничение не действует: так тесты и отдельные
	/// сервисы могут работать без загруженного профиля.
	bool allowsEncryptedStores() const;

	/// Выбирает профиль и создаёт его каталог
	/// (<path.profile>/profiles/<name>), если его ещё нет.
	bool selectProfile(const QString& name);

	using ItemsCollection = decltype(make_deref_view(std::declval<const std::list<std::unique_ptr<DataStreamContainer>>&>()));
	ItemsCollection items() const;

public:
	VariablesContext Variables;
	DirectoriesContext Directories;

private:
	class Private;
	std::unique_ptr<Private> d;
};
