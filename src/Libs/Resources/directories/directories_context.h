#pragma once
#include <QDir>
#include <QMap>
#include <QList>
#include <optional>

enum class DirectoryPath {
	ResourcesPath,
	AssetsPath,
	RecoveryWordsFile,
	DataPath,
	DatabasesConfigFile,
	UsersPath,
	CurrentUserPath,
	AutchFile,

	// --- Databases ----
	AccountsDbFile,
	UsersDbFile,
	GameDbFile,
	MessangerDbFile,
};

class DirectoriesContext {
public:
	std::optional<QDir> get(DirectoryPath path) const;
	std::optional<QDir> set(DirectoryPath path, const QDir& dir);
	void clear();
	void apply(const DirectoriesContext& values);

	QList<DirectoryPath> available() const;
	QStringList paths() const;

private:
	QMap<DirectoryPath, QDir> _directories;
};
