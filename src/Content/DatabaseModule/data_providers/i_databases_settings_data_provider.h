#pragma once
#include <QString>
#include <optional>

class IDatabaseSettingsDataProvider {
public:
	struct Settings {
		// интервал проверки Wal в сек
		int walCheckInterval = 60000;

		// мвксимальный размер wal файла в Мегабайтах
		int walMaxSizeMb = 20;
	};

	virtual ~IDatabaseSettingsDataProvider() = default;

	virtual std::optional<Settings> get(const QString& name) const = 0;
};
