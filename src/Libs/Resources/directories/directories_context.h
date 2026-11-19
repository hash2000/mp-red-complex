#pragma once
#include <QString>
#include <QDir>
#include <QMap>
#include <QList>

#include <optional>

class DirectoriesContext {
public:
	std::optional<QDir> get(const QString& path) const;
	std::optional<QDir> set(const QString& path, const QDir& dir);
	void clear();
	void apply(const DirectoriesContext& values);

	QList<QString> available() const;
	QStringList paths() const;

private:
	QMap<QString, QDir> _directories;
};
